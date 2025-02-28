#pragma once
#include <Windows.h>
#include <stdio.h>
#include <Pdh.h>
#pragma comment(lib, "Pdh.lib")

/***************************

	   CMonitor Class

***************************/
// ����͸��� �ʿ��� ����� �����ϴ� Ŭ�����̴�.
// CPU �����, �޸� ����� �� ���μ��� �� ���� ��ǻ�Ϳ� ���� ����͸��� �ʿ��� �κе��� ����͸��ϴ� ����� �����Ѵ�.

// �ֱ������� �ش� Ŭ������ Update�Լ��� ȣ���Ͽ� ���� �����Ͽ� Ȯ���Ѵ�.
// ���ÿ� ���� ��Ʈ���� ���α׷� �̸��� �����Ͽ� ����Ѵ�.

class CMonitor
{
public:
	CMonitor(HANDLE hProcess = INVALID_HANDLE_VALUE);

public:
	//---------------------------------------------------------
	// CPU ����� ���� �Լ�
	//---------------------------------------------------------
	void  UpdateCpuTime(void);
	float ProcessorTotal(void)	{ return _fProcessorTotal; }
	float ProcessorUser(void)	{ return _fProcessorUser; }
	float ProcessorKernel(void) { return _fProcessorKernel; }
	float ProcessTotal(void)	{ return _fProcessTotal; }
	float ProcessUser(void)		{ return _fProcessUser; }
	float ProcessKernel(void)	{ return _fProcessKernel; }


	//---------------------------------------------------------
	// �޸� ������ ���� �Լ�
	//---------------------------------------------------------
	double ProcessMemory()			{ return _processUserMemoryValue.doubleValue; }
	double ProcessNonpagedMemory()	{ return _processNonpagedMemoryValue.doubleValue; }
	double AvailableMemory()		{ return _availableMemoryValue.doubleValue; }
	double NonpagedMemory()			{ return _nonpagedMemoryValue.doubleValue; }

private:

	//---------------------------------------------------------
	// CPU ����� ���� ����
	//---------------------------------------------------------
	HANDLE _hProcess;
	int _iNumberOfProcessors;
	float _fProcessorTotal;
	float _fProcessorUser;
	float _fProcessorKernel;
	float _fProcessTotal;
	float _fProcessUser;
	float _fProcessKernel;

	ULARGE_INTEGER _ftProcessor_LastKernel;
	ULARGE_INTEGER _ftProcessor_LastUser;
	ULARGE_INTEGER _ftProcessor_LastIdle;
	ULARGE_INTEGER _ftProcess_LastKernel;
	ULARGE_INTEGER _ftProcess_LastUser;
	ULARGE_INTEGER _ftProcess_LastTime;

	//---------------------------------------------------------
	// �޸� ������ ���� ����
	//---------------------------------------------------------
	PDH_HCOUNTER   _processUserMemory;
	PDH_HCOUNTER   _processNonpagedMemory;
	PDH_HCOUNTER   _availableMemory;
	PDH_HCOUNTER   _nonpagedMemory;

	PDH_HQUERY			 _cpuQuery;
	PDH_FMT_COUNTERVALUE _processUserMemoryValue;
	PDH_FMT_COUNTERVALUE _processNonpagedMemoryValue;
	PDH_FMT_COUNTERVALUE _availableMemoryValue;
	PDH_FMT_COUNTERVALUE _nonpagedMemoryValue;
};

