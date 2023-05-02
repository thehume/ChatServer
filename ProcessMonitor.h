#pragma once

class CProcessMonitor
{
public:
	CProcessMonitor(HANDLE ProcessHandle = INVALID_HANDLE_VALUE);

	void Update(void);

	float getProcessTotal(void) { return ProcessTotal; }
	float getProcessUser(void) { return ProcessUser; }
	float getProcessKernel(void) { return ProcessKernel; }

	double getProcessUserMemory(void) { return ProcessUserMemoryCounterVal.doubleValue; }
	double getProcessNonpagedMemory(void) { return ProcessNonpagedMemoryCounterVal.doubleValue; }

private:
	HANDLE hProcess;
	int NumberOfProcessors;

	float ProcessTotal;
	float ProcessUser;
	float ProcessKernel;

	ULARGE_INTEGER Process_LastKernel;
	ULARGE_INTEGER Process_LastUser;
	ULARGE_INTEGER Process_LastTime;

	PDH_HQUERY ProcessQuery;
	PDH_HCOUNTER ProcessUserMemory;
	PDH_HCOUNTER ProcessNonpagedMemory;

	PDH_FMT_COUNTERVALUE ProcessUserMemoryCounterVal;
	PDH_FMT_COUNTERVALUE ProcessNonpagedMemoryCounterVal;
};