#pragma once

//채팅서버, 3개 클래스로 존재
//Handler 클래스, OnRecv 클래스, ChatServer 클래스
//Handler 클래스
//OnRecv 클래스 : ChatServer의 잡 큐에 Enqueue 해줌
//ChatServer 클래스 : 로직스레드 생성. 멤버로 유저 데이터 들고있음, 패킷처리함수(패킷 만들어서 보내기, 받은패킷 처리하기)
// 로직스레드는 무한 Dequeue 중. Dequeue 실패시 10~20ms 쉬고 다시 Dequeue 시도 or 폴링
// 유저 데이터는 unordered map 쓴다 치고 우선 데이터구조체 멤버 : sector X,Y, 세션ID, AccountNo. 고정 배열을 쓴다면 isValid 멤버까지? 
// 섹터도 구현해줘야함. MMO TCP fighter 참조

#define dfSECTOR_MAX_X 50
#define dfSECTOR_MAX_Y 50

struct st_JobItem
{
    INT64 SessionID;
    CPacket* pPacket;
};

struct st_Message
{
    WCHAR msg[50];
};

struct st_UserName
{
    WCHAR name[20];
};

struct st_SessionKey
{
    char sessionKey[64];
};


CPacket& operator << (CPacket& packet, st_UserName& userName)
{

    if (packet.GetLeftUsableSize() >= sizeof(st_UserName))
    {
        memcpy(packet.GetWriteBufferPtr(), userName.name, sizeof(st_UserName));
        packet.MoveWritePos(sizeof(st_UserName));
        packet.AddDataSize(sizeof(st_UserName));
    }
    return packet;
}

CPacket& operator << (CPacket& packet, st_Message& Message)
{

    if (packet.GetLeftUsableSize() >= sizeof(st_Message))
    {
        memcpy(packet.GetWriteBufferPtr(), Message.msg, sizeof(st_Message));
        packet.MoveWritePos(sizeof(st_Message));
        packet.AddDataSize(sizeof(st_Message));
    }
    return packet;
}

CPacket& operator << (CPacket& packet, st_SessionKey& SessionKey)
{

    if (packet.GetLeftUsableSize() >= sizeof(st_SessionKey))
    {
        memcpy(packet.GetWriteBufferPtr(), SessionKey.sessionKey, sizeof(st_SessionKey));
        packet.MoveWritePos(sizeof(st_SessionKey));
        packet.AddDataSize(sizeof(st_SessionKey));
    }
    return packet;
}

CPacket& operator >> (CPacket& packet, st_UserName& userName)
{
    if (packet.GetDataSize() >= sizeof(st_UserName))
    {
        memcpy(userName.name, packet.GetReadBufferPtr(), sizeof(st_UserName));
        packet.MoveReadPos(sizeof(st_UserName));
        packet.SubDataSize(sizeof(st_UserName));
    }
    return packet;
}

CPacket& operator >> (CPacket& packet, st_Message& Message)
{
    if (packet.GetDataSize() >= sizeof(st_Message))
    {
        memcpy(Message.msg, packet.GetReadBufferPtr(), sizeof(st_Message));
        packet.MoveReadPos(sizeof(st_Message));
        packet.SubDataSize(sizeof(st_Message));
    }
    return packet;
}

CPacket& operator >> (CPacket& packet, st_SessionKey& SessionKey)
{
    if (packet.GetDataSize() >= sizeof(st_SessionKey))
    {
        memcpy(SessionKey.sessionKey, packet.GetReadBufferPtr(), sizeof(st_SessionKey));
        packet.MoveReadPos(sizeof(st_SessionKey));
        packet.SubDataSize(sizeof(st_SessionKey));
    }
    return packet;
}


class CChatServer;

class CContentsHandler : public CNetServerHandler
{
public:
    void attachServerInstance(CNetServer* networkServer, CChatServer* contentsServer)
    {
        pNetServer = networkServer;
        pChatServer = contentsServer;
    }
    virtual bool OnConnectionRequest() { return true; }
    virtual void OnClientJoin(INT64 sessionID)
    {
        short index = (short)sessionID;
        //pChatServer->PlayerList[index].isValid = TRUE; 인터락이 좋을까?
        pChatServer->PlayerList[index].sessionID = sessionID;
    }

    virtual void OnClientLeave(INT64 sessionID)
    {
        short index = (short)sessionID;
        pChatServer->PlayerList[index].isValid = FALSE; //인터락이 좋을까?
    }
    virtual void OnError(int errorCode)
    {

    }

private:
    CNetServer* pNetServer;
    CChatServer* pChatServer;
};

class CContentsRPC : public CNetServerIRPC
{
public:

    void attachServerInstance(CNetServer* networkServer, CChatServer* contentsServer)
    {
        pNetServer = networkServer;
        pChatServer = contentsServer;
    }

    virtual bool OnRecv(INT64 SessionID, CPacket* pPacket) //우선 시그널링방식은 아님! 채팅서버가 폴링을 할꺼기때문
    {
        pPacket->addRef(1);
        st_JobItem jobItem;
        jobItem.SessionID = SessionID;
        jobItem.pPacket = pPacket;
        if (pChatServer->JobQueue.Enqueue(jobItem) == false) 
        {
            if (pPacket->subRef() == 0)
            {
                CPacket::mFree(pPacket);
            }
            return false;
        }

        return true;
    }

private:
    CNetServer* pNetServer;
    CChatServer* pChatServer;
};

class CChatServer
{
    friend class CContentsHandler;
    friend class CContentsRPC;

public:

    struct st_SectorPos
    {
        WORD sectorX;
        WORD sectorY;
    };

    struct st_Player
    {
        BOOL isValid;
        INT64 AccountNo;
        st_UserName ID;
        st_UserName Nickname;
        st_SectorPos sectorPos;
        INT64 sessionID;
        st_SessionKey sessionKey;
    };

    static DWORD WINAPI LogicThread(CChatServer* pChatServer);
    //static 쓰레드함수 한개 : Dequeue해서 로직타는루프
    //패킷 프로시저들!!
    void CS_CHAT_RES_LOGIN(INT64 SessionID, BYTE Status, INT64 AccountNo);
    void CS_CHAT_RES_SECTOR_MOVE(INT64 SessionID, INT64 AccountNo, WORD SectorX, WORD	SectorY);
    void CS_CHAT_RES_MESSAGE(INT64 SessionID, INT64 AccountNo, st_UserName ID, st_UserName Nickname, WORD MessageLen, st_Message Message);


    bool packetProc_CS_CHAT_REQ_LOGIN(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_SECTOR_MOVE(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_MESSAGE(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_HEARTBEAT(st_Player* pPlayer, CPacket* pPacket);

    bool PacketProc(st_Player* pPlayer, WORD PacketType, CPacket* pPacket);

    //생성자에서 쓰레드 만들기
    //잡큐

    /*
    void getCharacterNum(void); // 캐릭터수
    void sector_AddCharacter(st_Character* pCharacter); //섹터에 캐릭터 넣음
    void sector_RemoveCharacter(st_Character* pCharacter); //섹터에서 캐릭터 삭제

    void getSectorAround(int sectorX, int sectorY, st_SectorAround* pSectorAround); //현재섹터 기준으로 9개섹터

    //"나" 기준으로 주위섹터의 세션 셋 가져옴
    void makeSessionSet_AroundMe(st_Character* pCharacter, CSessionSet* InParamSet, bool sendMe = false);
    */

private:
    CNetServer* pNetServer;
    st_Player PlayerList[dfMAX_SESSION];
    list <st_Player*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
    LockFreeQueue<st_JobItem> JobQueue;
};