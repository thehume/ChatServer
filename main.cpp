#pragma comment(lib, "winmm.lib" )
#pragma comment(lib, "ws2_32")
#pragma comment(lib,"Pdh.lib")
#include <winsock2.h>
#include <ws2tcpip.h>
#include <dbghelp.h>
#include <list>
#include <locale.h>
#include <random>
#include <process.h>
#include <stdlib.h>
#include <iostream>
#include <Pdh.h>
#include <strsafe.h>
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
#include "HardwareMonitor.h"
#include "ProcessMonitor.h"

using namespace std;

CrashDump myDump;

WCHAR IPaddress[20] = L"0.0.0.0";
CInitParam initParam(IPaddress, 6000, 6, 3, true, 15000);
CNetServer NetServer(&initParam);
CChatServer ChatServer;

CHardwareMonitor Hardware_Monitor;
CProcessMonitor Process_Monitor(GetCurrentProcess());


int main()
{

	CContentsHandler HandleInstance;
	HandleInstance.attachServerInstance(&NetServer, &ChatServer);

	NetServer.attachHandler(&HandleInstance);
	ChatServer.attachServerInstance(&NetServer);

	ChatServer.Start();
	NetServer.Start();
	int i = 10;

	while (1)
	{
		Hardware_Monitor.Update();
		Process_Monitor.Update();

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
		wprintf(L"Process User Memory : %d Bytes\n", (int)Process_Monitor.getProcessUserMemory());
		wprintf(L"Process Nonpaged Memory : %d Bytes\n", (int)Process_Monitor.getProcessNonpagedMemory());
		wprintf(L"Process : %f %%, ", Process_Monitor.getProcessTotal());
		wprintf(L"ProcessKernel : %f %%, ", Process_Monitor.getProcessKernel());
		wprintf(L"ProcessUser : %f %%\n", Process_Monitor.getProcessUser());
		wprintf(L"======================\n");
		wprintf(L"Available Memory : %d MBytes\n", (int)Hardware_Monitor.getAvailableMemory());
		wprintf(L"Nonpaged Memory : %d Bytes\n", (int)Hardware_Monitor.getNonpagedMemory());
		wprintf(L"Processor : %f%%, ", Hardware_Monitor.getProcessorTotal());
		wprintf(L"ProcessorKernel : %f%%, ", Hardware_Monitor.getProcessorKernel());
		wprintf(L"ProcessorUser : %f%% \n", Hardware_Monitor.getProcessorUser());
		wprintf(L"NetWork RecvBytes : %d Bytes\n", (int)Hardware_Monitor.getRecvBytes());
		wprintf(L"NetWork SendBytes : %d Bytes\n", (int)Hardware_Monitor.getSendBytes());
		wprintf(L"======================\n");
		Sleep(1000);
		//i--;
	}

	ChatServer.Stop();
	NetServer.Stop();


	return 0;
}