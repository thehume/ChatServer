#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
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
#include "CNetServer.h"
#include "ChatServer.h"
#include "CommonProtocol.h"

using namespace std;

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

}
void CChatServer::CS_CHAT_RES_MESSAGE(INT64 SessionID, INT64 AccountNo, st_UserName ID, st_UserName Nickname, WORD MessageLen, st_Message Message)
{

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
	//init flag 대기중

	st_JobItem jobItem;
	WORD packetType;
	while (1) // 일단 폴링(shutdown 등의 flag로 종료)
	{
		if (pChatServer->JobQueue.Dequeue(&jobItem) == false) //packet의 참조카운트는 올리면서 enqueue됨. 사용했으면 내려줘야함
		{
			continue;
		}

		short index = (short)jobItem.SessionID;
		CPacket* pPacket = jobItem.pPacket;

		if (pChatServer->PlayerList[index].sessionID != jobItem.SessionID )
		{
			if (pPacket->subRef() == 0)
			{
				CPacket::mFree(pPacket);
			}
			continue; // 예외처리는 좀더 생각해보기
		}

		if (pChatServer->PlayerList[index].isValid == FALSE)
		{
			if (pPacket->subRef() == 0)
			{
				CPacket::mFree(pPacket);
			}
			continue;
		}

		//pPacket에서 type 뺀다.
		*pPacket >> packetType;
		bool ret = pChatServer->PacketProc(&pChatServer->PlayerList[index], packetType, pPacket);
		if (ret == false)
		{
			//끊기요청
		}
		

		//패킷 프로시져 타기



		if (pPacket->subRef() == 0)
		{
			CPacket::mFree(pPacket);
		}

	}
}