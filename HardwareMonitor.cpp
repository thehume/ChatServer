#pragma comment(lib,"Pdh.lib")
#include <Windows.h>
#include <Pdh.h>
#include <strsafe.h>
#include <iostream>
#include "HardwareMonitor.h"

CHardwareMonitor::CHardwareMonitor()
{
	//프로세서 개수 확인
	SYSTEM_INFO SystemInfo;

	GetSystemInfo(&SystemInfo);
	NumberOfProcessors = SystemInfo.dwNumberOfProcessors;

	ProcessorTotal = 0;
	ProcessorUser = 0;
	ProcessorKernel = 0;

	Processor_LastKernel.QuadPart = 0;
	Processor_LastUser.QuadPart = 0;
	Processor_LastIdle.QuadPart = 0;

	PdhOpenQuery(NULL, NULL, &HardwareQuery);
	PdhAddCounter(HardwareQuery, L"\\Memory\\Available MBytes", NULL, &AvailableMemory);
	PdhAddCounter(HardwareQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &NonpagedMemory);

	PdhCollectQueryData(HardwareQuery);

	//이더넷
	PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD, 0);
	szCounters = new WCHAR[dwCounterSize];
	szInterfaces = new WCHAR[dwInterfaceSize];

	if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", szCounters, &dwCounterSize, szInterfaces, &dwInterfaceSize, PERF_DETAIL_WIZARD,
		0) != ERROR_SUCCESS)
	{
		delete[] szCounters;
		delete[] szInterfaces;
	}

	else
	{
		iCnt = 0;
		szCur = szInterfaces;

		for (; *szCur != L'\0' && iCnt < df_PDH_ETHERNET_MAX; szCur += wcslen(szCur) + 1, iCnt++)
		{
			_EthernetStruct[iCnt]._bUse = true;
			_EthernetStruct[iCnt]._szName[0] = L'\0';
			wcscpy_s(_EthernetStruct[iCnt]._szName, szCur);
			szQuery[0] = L'\0';
			StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", szCur);
			PdhAddCounter(HardwareQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes);
			szQuery[0] = L'\0';
			StringCbPrintf(szQuery, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", szCur);
			PdhAddCounter(HardwareQuery, szQuery, NULL, &_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes);
		}
	}




	Update();
}

void CHardwareMonitor::Update()
{
	ULARGE_INTEGER Idle;
	ULARGE_INTEGER Kernel;
	ULARGE_INTEGER User;

	if (GetSystemTimes((PFILETIME)&Idle, (PFILETIME)&Kernel, (PFILETIME)&User) == false)
	{
		return;
	}

	ULONGLONG KernelDiff = Kernel.QuadPart - Processor_LastKernel.QuadPart;
	ULONGLONG UserDiff = User.QuadPart - Processor_LastUser.QuadPart;
	ULONGLONG IdleDiff = Idle.QuadPart - Processor_LastIdle.QuadPart;

	ULONGLONG Total = KernelDiff + UserDiff;

	if (Total == 0)
	{
		ProcessorUser = 0.0f;
		ProcessorKernel = 0.0f;
		ProcessorTotal = 0.0f;
	}

	else
	{
		ProcessorTotal = (float)((double)(Total - IdleDiff) / Total * 100.0f);
		ProcessorUser = (float)((double)UserDiff / Total * 100.0f);
		ProcessorKernel = (float)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
	}

	Processor_LastKernel = Kernel;
	Processor_LastUser = User;
	Processor_LastIdle = Idle;

	//갱신
	PdhCollectQueryData(HardwareQuery);

	// 갱신 데이터 얻음
	PdhGetFormattedCounterValue(AvailableMemory, PDH_FMT_DOUBLE, NULL, &AvailableMemoryCounterVal);
	PdhGetFormattedCounterValue(NonpagedMemory, PDH_FMT_DOUBLE, NULL, &NonpagedMemoryCounterVal);


	//이더넷
	_pdh_value_Network_RecvBytes = 0;
	_pdh_value_Network_SendBytes = 0;

	for (int iCnt = 0; iCnt < df_PDH_ETHERNET_MAX; iCnt++)
	{
		if (_EthernetStruct[iCnt]._bUse)
		{
			PDH_STATUS Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_RecvBytes,
				PDH_FMT_DOUBLE, NULL, &EthernetCounterVal);
			if (Status == 0) _pdh_value_Network_RecvBytes += EthernetCounterVal.doubleValue;
			Status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._pdh_Counter_Network_SendBytes,
				PDH_FMT_DOUBLE, NULL, &EthernetCounterVal);
			if (Status == 0) _pdh_value_Network_SendBytes += EthernetCounterVal.doubleValue;
		}
	}
}