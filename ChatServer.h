#pragma once

//ä�ü���, 3�� Ŭ������ ����
//Handler Ŭ����, OnRecv Ŭ����, ChatServer Ŭ����
//Handler Ŭ����
//OnRecv Ŭ���� : ChatServer�� �� ť�� Enqueue ����
//ChatServer Ŭ���� : ���������� ����. ����� ���� ������ �������, ��Ŷó���Լ�(��Ŷ ���� ������, ������Ŷ ó���ϱ�)
// ����������� ���� Dequeue ��. Dequeue ���н� 10~20ms ���� �ٽ� Dequeue �õ� or ����
// ���� �����ʹ� unordered map ���� ġ�� �켱 �����ͱ���ü ��� : sector X,Y, ����ID, AccountNo. ���� �迭�� ���ٸ� isValid �������? 
// ���͵� �����������. MMO TCP fighter ����

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
        //pChatServer->PlayerList[index].isValid = TRUE; ���Ͷ��� ������?
        pChatServer->PlayerList[index].sessionID = sessionID;
    }

    virtual void OnClientLeave(INT64 sessionID)
    {
        short index = (short)sessionID;
        pChatServer->PlayerList[index].isValid = FALSE; //���Ͷ��� ������?
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

    virtual bool OnRecv(INT64 SessionID, CPacket* pPacket) //�켱 �ñ׳θ������ �ƴ�! ä�ü����� ������ �Ҳ��⶧��
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
    //static �������Լ� �Ѱ� : Dequeue�ؼ� ����Ÿ�·���
    //��Ŷ ���ν�����!!
    void CS_CHAT_RES_LOGIN(INT64 SessionID, BYTE Status, INT64 AccountNo);
    void CS_CHAT_RES_SECTOR_MOVE(INT64 SessionID, INT64 AccountNo, WORD SectorX, WORD	SectorY);
    void CS_CHAT_RES_MESSAGE(INT64 SessionID, INT64 AccountNo, st_UserName ID, st_UserName Nickname, WORD MessageLen, st_Message Message);


    bool packetProc_CS_CHAT_REQ_LOGIN(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_SECTOR_MOVE(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_MESSAGE(st_Player* pPlayer, CPacket* pPacket);
    bool packetProc_CS_CHAT_REQ_HEARTBEAT(st_Player* pPlayer, CPacket* pPacket);

    bool PacketProc(st_Player* pPlayer, WORD PacketType, CPacket* pPacket);

    //�����ڿ��� ������ �����
    //��ť

    /*
    void getCharacterNum(void); // ĳ���ͼ�
    void sector_AddCharacter(st_Character* pCharacter); //���Ϳ� ĳ���� ����
    void sector_RemoveCharacter(st_Character* pCharacter); //���Ϳ��� ĳ���� ����

    void getSectorAround(int sectorX, int sectorY, st_SectorAround* pSectorAround); //���缽�� �������� 9������

    //"��" �������� ���������� ���� �� ������
    void makeSessionSet_AroundMe(st_Character* pCharacter, CSessionSet* InParamSet, bool sendMe = false);
    */

private:
    CNetServer* pNetServer;
    st_Player PlayerList[dfMAX_SESSION];
    list <st_Player*> g_Sector[dfSECTOR_MAX_Y][dfSECTOR_MAX_X];
    LockFreeQueue<st_JobItem> JobQueue;
};