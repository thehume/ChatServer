#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <list>
#include <locale.h>
#include <random>
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
#include "CommonProtocol.h"
#include "ChatServer.h"

using namespace std;

CrashDump myDump;

WCHAR IPaddress[20] = L"0.0.0.0";
CInitParam initParam(IPaddress, 6000, 12, 4, true, 15000);
CNetServer NetServer(&initParam);
CChatServer ChatServer;


int main()
{
	/*
	char dataArray[55] = "aaaaaaaaaabbbbbbbbbbcccccccccc1234567890abcdefghijklmn";

	st_header myheader;
	myheader.code = 0xa9;
	myheader.len = 55;
	myheader.randkey = 0x31;
	myheader.checksum = 0;

	CPacket* Buf = CPacket::mAlloc();
	Buf->ClearNetwork();
	Buf->PutData((char*)&myheader, 5);
	Buf->PutData(dataArray, 55);
	Buf->Encode();
	Buf->Decode();

	Sleep(10000000);
	*/

	CContentsHandler HandleInstance;
	HandleInstance.attachServerInstance(&NetServer, &ChatServer);

	NetServer.attachHandler(&HandleInstance);
	ChatServer.attachServerInstance(&NetServer);

	ChatServer.Start();
	NetServer.Start();
	int i = 10;

	while (1)
	{
		wprintf(L"======================\n");
		wprintf(L"session number : %d\n", NetServer.getSessionCount());
		wprintf(L"Character Number : %lld\n", ChatServer.getCharacterNum());
		wprintf(L"Accept Sum : %lld\n", NetServer.getAcceptSum());
		wprintf(L"Accept TPS : %d\n", NetServer.getAcceptTPS());
		wprintf(L"Disconnect TPS : %d\n", NetServer.getDisconnectTPS());
		wprintf(L"Send TPS : %d\n", NetServer.getSendMessageTPS());
		wprintf(L"Recv TPS : %d\n", NetServer.getRecvMessageTPS());
		wprintf(L"JobQueue UseSize : %d\n", ChatServer.getJobQueueUseSize());
		wprintf(L"PacketPool UseSize : %d\n", CPacket::getPoolUseSize() * POOL_BUCKET_SIZE);
		wprintf(L"======================\n");
		Sleep(1000);
		//i--;
	}

	ChatServer.Stop();
	NetServer.Stop();


	return 0;
}