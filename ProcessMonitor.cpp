#pragma comment(lib,"Pdh.lib")
#include <Windows.h>
#include <Pdh.h>
#include "ProcessMonitor.h"

CProcessMonitor::CProcessMonitor(HANDLE ProcessHandle)
{
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		hProcess = GetCurrentProcess();
	}

	hProcess = ProcessHandle;
	//프로세서 개수 확인
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);
	NumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	ProcessTotal = 0;
	ProcessUser = 0;
	ProcessKernel = 0;

	Process_LastKernel.QuadPart = 0;
	Process_LastUser.QuadPart = 0;
	Process_LastTime.QuadPart = 0;

	PdhOpenQuery(NULL, NULL, &ProcessQuery);
	PdhAddCounter(ProcessQuery, L"\\Process(ChatServer)\\Private Bytes", NULL, &ProcessUserMemory);
	PdhAddCounter(ProcessQuery, L"\\Process(ChatServer)\\Pool Nonpaged Bytes", NULL, &ProcessNonpagedMemory);

	PdhCollectQueryData(ProcessQuery);

	Update();
}

void CProcessMonitor::Update()
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}

	ULONGLONG Total;
	ULONGLONG TimeDiff;
	ULONGLONG UserDiff;
	ULONGLONG KernelDiff;

	ULARGE_INTEGER None;
	ULARGE_INTEGER NowTime;

	GetSystemTimeAsFileTime((LPFILETIME)&NowTime);
	GetProcessTimes(hProcess, (LPFILETIME)&None, (LPFILETIME)&None, (LPFILETIME)&Kernel, (LPFILETIME)&User);

	TimeDiff = NowTime.QuadPart - Process_LastTime.QuadPart;
	UserDiff = User.QuadPart - Process_LastUser.QuadPart;
	KernelDiff = Kernel.QuadPart - Process_LastKernel.QuadPart;

	Total = KernelDiff + UserDiff;

	ProcessTotal = (float)(Total / (double)NumberOfProcessors / (double)TimeDiff * 100.0f);
	ProcessKernel = (float)(KernelDiff / (double)NumberOfProcessors / (double)TimeDiff * 100.0f);
	ProcessUser = (float)(UserDiff / (double)NumberOfProcessors / (double)TimeDiff * 100.0f);

	Process_LastTime = NowTime;
	Process_LastKernel = Kernel;
	Process_LastUser = User;

	//갱신
	PdhCollectQueryData(ProcessQuery);

	// 갱신 데이터 얻음
	PdhGetFormattedCounterValue(ProcessUserMemory, PDH_FMT_DOUBLE, NULL, &ProcessUserMemoryCounterVal);
	PdhGetFormattedCounterValue(ProcessNonpagedMemory, PDH_FMT_DOUBLE, NULL, &ProcessNonpagedMemoryCounterVal);
}