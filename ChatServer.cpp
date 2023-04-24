#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <list>
#include <random>
#include <locale.h>
#include <process.h>
#include <stdlib.h>
#include <iostream>
#include <unordered_map>
#include "ringbuffer.h"
#include "MemoryPoolBucket.h"
#include "Packet.h"
#include "profiler.h"
#include "dumpClass.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "CommonProtocol.h"
#include "CNetServer.h"
#include "ChatServer.h"

using namespace std;

CChatServer::CChatServer()
{
	ShutDownFlag = false;
	lastTime = 0;
	pNetServer = NULL;

	for (int i = 0; i < dfMAX_SESSION; i++)
	{
		PlayerList[i].isValid = FALSE;
		PlayerList[i].AccountNo = 0;
		wcscpy_s(PlayerList[i].ID.name, L"NULL");
		wcscpy_s(PlayerList[i].Nickname.name, L"NULL");
		PlayerList[i].sectorPos.sectorX = -1;
		PlayerList[i].sectorPos.sectorY = -1;
		PlayerList[i].sessionID = 0;
		PlayerList[i].lastTime = 0;
	}
}


void CChatServer::CS_CHAT_RES_LOGIN(INT64 SessionID, BYTE Status, INT64 AccountNo)
{
	WORD Type = en_PACKET_CS_CHAT_RES_LOGIN;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->addRef(1);

	*pPacket << Type;
	*pPacket << Status;
	*pPacket << AccountNo;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}
void CChatServer::CS_CHAT_RES_SECTOR_MOVE(INT64 SessionID, INT64 AccountNo, WORD SectorX, WORD SectorY)
{
	WORD Type = en_PACKET_CS_CHAT_RES_SECTOR_MOVE;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->addRef(1);

	*pPacket << Type;
	*pPacket << AccountNo;
	*pPacket << SectorX;
	*pPacket << SectorY;

	pNetServer->sendPacket(SessionID, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

//세션 셋으로 바꿀꺼다
void CChatServer::CS_CHAT_RES_MESSAGE(CSessionSet* SessionSet, INT64 AccountNo, st_UserName ID, st_UserName Nickname, WORD MessageLen, st_Message& Message)
{
	WORD Type = en_PACKET_CS_CHAT_RES_MESSAGE;

	CPacket* pPacket = CPacket::mAlloc();
	pPacket->addRef(1);

	*pPacket << Type;
	*pPacket << AccountNo;
	*pPacket << ID;
	*pPacket << Nickname;
	*pPacket << MessageLen;
	*pPacket << Message;


	pNetServer->sendPacket(SessionSet, pPacket);
	if (pPacket->subRef() == 0)
	{
		CPacket::mFree(pPacket);
	}
}

bool CChatServer::packetProc_CS_CHAT_REQ_LOGIN(st_Player* pPlayer, CPacket* pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 로그인 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WCHAR	ID[20]				// null 포함
	//		WCHAR	Nickname[20]		// null 포함
	//		char	SessionKey[64];		// 인증토큰
	//	}
	//
	//------------------------------------------------------------
	INT64 AccountNo;
	st_UserName ID;
	st_UserName Nickname;
	st_SessionKey SessionKey;
	*pPacket >> AccountNo >> ID >> Nickname >> SessionKey;
	
	pPlayer->AccountNo = AccountNo;
	memcpy(&pPlayer->ID, ID.name, sizeof(st_UserName));
	memcpy(&pPlayer->Nickname, Nickname.name, sizeof(st_UserName));
	memcpy(&pPlayer->sessionKey, SessionKey.sessionKey, sizeof(st_SessionKey));

	pPlayer->isValid = TRUE;

	//응답패킷 보내기.
	CS_CHAT_RES_LOGIN(pPlayer->sessionID, TRUE, AccountNo);

	return true;
}
bool CChatServer::packetProc_CS_CHAT_REQ_SECTOR_MOVE(st_Player* pPlayer, CPacket* pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 섹터 이동 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	SectorX
	//		WORD	SectorY
	//	}
	//
	//------------------------------------------------------------
	INT64 AccountNo;
	WORD SectorX;
	WORD SectorY;

	*pPacket >> AccountNo >> SectorX >> SectorY;

	if (pPlayer->AccountNo != AccountNo)
	{
		//로그찍기
		return false;
	}

	if (SectorX >= 50 || SectorX < 0 || SectorY >= 50 || SectorY < 0)
	{
		//로그찍기
		return false;
	}


	//현재섹터에서 나 삭제
	sector_RemoveCharacter(pPlayer);

	//나의 섹터정보 업데이트
	pPlayer->sectorPos.sectorX = SectorX;
	pPlayer->sectorPos.sectorY = SectorY;

	//바뀔 섹터에 나 넣어주기
	sector_AddCharacter(pPlayer);

	//업데이트된 섹터 메시지 보내주기
	CS_CHAT_RES_SECTOR_MOVE(pPlayer->sessionID, AccountNo, SectorX, SectorY);
	
	return true;
}


bool CChatServer::packetProc_CS_CHAT_REQ_MESSAGE(st_Player* pPlayer, CPacket* pPacket)
{
	//------------------------------------------------------------
	// 채팅서버 채팅보내기 요청
	//
	//	{
	//		WORD	Type
	//
	//		INT64	AccountNo
	//		WORD	MessageLen
	//		WCHAR	Message[MessageLen / 2]		// null 미포함
	//	}
	//
	//------------------------------------------------------------
	INT64 AccountNo;
	WORD MessageLen;
	st_Message Message;
	*pPacket >> AccountNo >> MessageLen;
	Message.len = MessageLen;
	*pPacket >> Message;

	if (pPlayer->AccountNo != AccountNo)
	{
		//로그찍기
		return false;
	}

	CSessionSet sessionSet;
	makeSessionSet_AroundMe(pPlayer, &sessionSet);

	CS_CHAT_RES_MESSAGE(&sessionSet, AccountNo, pPlayer->ID, pPlayer->Nickname, MessageLen, Message);

	return true;

}
bool CChatServer::packetProc_CS_CHAT_REQ_HEARTBEAT(st_Player* pPlayer, CPacket* pPacket)
{
	//------------------------------------------------------------
	// 하트비트
	//
	//	{
	//		WORD		Type
	//	}
	//
	//
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
	//------------------------------------------------------------	

	return true;

}


bool CChatServer::PacketProc(st_Player* pPlayer, WORD PacketType, CPacket* pPacket)
{
	switch (PacketType)
	{
	case en_PACKET_CS_CHAT_REQ_LOGIN:
		return packetProc_CS_CHAT_REQ_LOGIN(pPlayer, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_SECTOR_MOVE:
		return packetProc_CS_CHAT_REQ_SECTOR_MOVE(pPlayer, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_MESSAGE:
		return packetProc_CS_CHAT_REQ_MESSAGE(pPlayer, pPacket);
		break;

	case en_PACKET_CS_CHAT_REQ_HEARTBEAT:
		return packetProc_CS_CHAT_REQ_HEARTBEAT(pPlayer, pPacket);
		break;

	default:
		return false;
	}
}


DWORD WINAPI CChatServer::LogicThread(CChatServer* pChatServer)
{
	st_JobItem jobItem;
	WORD packetType;
	while (!pChatServer->ShutDownFlag) // 일단 폴링
	{
		//시간 쟤서 약 1초마다 모든세션의 lastPacket 확인 -> 40초가 지났다면 그세션끊기
		ULONGLONG curTime = GetTickCount64();
		if (curTime - pChatServer->lastTime > 1000)
		{
			pChatServer->lastTime = curTime;
			st_Session* pSession;
			for (int i = 0; i < pChatServer->maxPlayer; i++)
			{
				st_Session* pSession;
				st_Player& player = pChatServer->PlayerList[i];
				if (player.isValid == FALSE)
				{
					continue;
				}
				if (curTime - player.lastTime > 40000)
				{
					if (pChatServer->pNetServer->findSession(player.sessionID, &pSession) == true)
					{
						pChatServer->pNetServer->disconnectSession(pSession);
						if (InterlockedDecrement(&pSession->IOcount) == 0)
						{
							pChatServer->pNetServer->releaseSession(player.sessionID);
						}
					}
				}
			}

		}

		if (pChatServer->JobQueue.Dequeue(&jobItem) == false) //packet의 참조카운트는 올리면서 enqueue됨. 사용했으면 내려줘야함
		{
			continue;
		}

		short index = (short)jobItem.SessionID;
		CPacket* pPacket = jobItem.pPacket;
		st_Player& player = pChatServer->PlayerList[index];

		if (pPacket == NULL) // OnclientLeave Job
		{
			pChatServer->sector_RemoveCharacter(&player);
			continue;
		}

		// OnRecv Job
		if (player.sessionID != jobItem.SessionID )
		{
			if (pPacket->subRef() == 0)
			{
				CPacket::mFree(pPacket);
			}
			continue; // 예외처리는 좀더 생각해보기
		}

		if (player.isValid == FALSE)
		{
			if (pPacket->subRef() == 0)
			{
				CPacket::mFree(pPacket);
			}
			continue;
		}

		//pPacket에서 type 뺀다.
		*pPacket >> packetType;

		//패킷 프로시져 타기
		player.lastTime = GetTickCount64();
		bool ret = pChatServer->PacketProc(&player, packetType, pPacket);
		if (ret == false)
		{
			//아래부분 함수로 래핑
			st_Session* pSession;
			if (pChatServer->pNetServer->findSession(player.sessionID, &pSession) == true)
			{
				pChatServer->pNetServer->disconnectSession(pSession);
				if (InterlockedDecrement(&pSession->IOcount) == 0)
				{
					pChatServer->pNetServer->releaseSession(player.sessionID);
				}
			}
		}

		if (pPacket->subRef() == 0)
		{
			CPacket::mFree(pPacket);
		}

	}
	return true;
}

bool CChatServer::Start()
{
	maxPlayer = pNetServer->getMaxSession();
	hLogicThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)&LogicThread, this, 0, 0);
	if (hLogicThread == NULL)
	{
		wprintf(L"LogicThread init error");
		return false;
	}

	return true;
}

bool CChatServer::Stop()
{
	ShutDownFlag = true;
	WaitForSingleObject(hLogicThread, INFINITE);
	return true;
}

void CChatServer::getCharacterNum(void) // 캐릭터수
{

}
void CChatServer::sector_AddCharacter(st_Player* pPlayer) //섹터에 캐릭터 넣음
{
	short Xpos = pPlayer->sectorPos.sectorX;
	short Ypos = pPlayer->sectorPos.sectorY;
	g_Sector[Ypos][Xpos].push_back(pPlayer);
}
void CChatServer::sector_RemoveCharacter(st_Player* pPlayer) //섹터에서 캐릭터 삭제
{
	short Xpos = pPlayer->sectorPos.sectorX;
	short Ypos = pPlayer->sectorPos.sectorY;
	list<st_Player*>::iterator iter = g_Sector[Ypos][Xpos].begin();
	for (; iter != g_Sector[Ypos][Xpos].end(); )
	{
		if (*iter == pPlayer)
		{
			iter = g_Sector[Ypos][Xpos].erase(iter);
			return;
		}
		else
		{
			iter++;
		}
	}
}

void CChatServer::getSectorAround(int sectorX, int sectorY, st_SectorAround* pSectorAround)
{
	int Xoffset, Yoffset;

	sectorX--;
	sectorY--;

	pSectorAround->count = 0;
	for (Yoffset = 0; Yoffset < 3; Yoffset++)
	{
		if (sectorY + Yoffset < 0 || sectorY + Yoffset >= dfSECTOR_MAX_Y)
		{
			continue;
		}

		for (Xoffset = 0; Xoffset < 3; Xoffset++)
		{
			if (sectorX + Xoffset < 0 || sectorX + Xoffset >= dfSECTOR_MAX_X)
			{
				continue;
			}
			pSectorAround->around[pSectorAround->count].sectorX = sectorX + Xoffset;
			pSectorAround->around[pSectorAround->count].sectorY = sectorY + Yoffset;
			pSectorAround->count++;
		}
	}
}

void CChatServer::makeSessionSet_AroundMe(st_Player* pPlayer, CSessionSet* InParamSet, bool sendMe)
{
	//CProfiler("makeSessionSet_AroundMe");
	st_SectorAround AroundMe;
	int sectorX, sectorY;
	getSectorAround(pPlayer->sectorPos.sectorX, pPlayer->sectorPos.sectorY, &AroundMe);
	for (int i = 0; i < AroundMe.count; i++)
	{
		sectorX = AroundMe.around[i].sectorX;
		sectorY = AroundMe.around[i].sectorY;

		list<st_Player*>& targetSector = g_Sector[sectorY][sectorX];
		list<st_Player*>::iterator sectorIter;

		for (sectorIter = targetSector.begin(); sectorIter != targetSector.end(); sectorIter++)
		{
			if (sendMe == false && *sectorIter == pPlayer)
			{
				continue;
			}
			InParamSet->setSession((*sectorIter)->sessionID);
		}

	}
}