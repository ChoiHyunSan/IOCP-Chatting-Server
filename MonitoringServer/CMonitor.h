#pragma once
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <Pdh.h>
#include <strsafe.h>
#pragma comment(lib, "Pdh.lib")

/***************************

	   CMonitor Class

***************************/
// 모니터링에 필요한 기능을 제공하는 클래스이다.
// CPU 사용율, 메모리 사용율 등 프로세스 및 서버 컴퓨터에 대한 모니터링이 필요한 부분들을 모니터링하는 기능을 지원한다.

// 주기적으로 해당 클래스의 Update함수를 호출하여 값을 갱신하여 확인한다.
// 사용시에 쿼리 스트링의 프로그램 이름을 수정하여 사용한다.

class CMonitor
{
public:
	CMonitor(HANDLE hProcess = INVALID_HANDLE_VALUE);

public:
	//---------------------------------------------------------
	// CPU 사용율 관련 함수
	//---------------------------------------------------------
	void  UpdateCpuTime(void);
	float ProcessorTotal(void)	{ return _fProcessorTotal; }
	float ProcessorUser(void)	{ return _fProcessorUser; }
	float ProcessorKernel(void) { return _fProcessorKernel; }
	float ProcessTotal(void)	{ return _fProcessTotal; }
	float ProcessUser(void)		{ return _fProcessUser; }
	float ProcessKernel(void)	{ return _fProcessKernel; }


	//---------------------------------------------------------
	// 메모리 가용율 관련 함수
	//---------------------------------------------------------
	double ProcessMemory()			{ return _processUserMemoryValue.doubleValue; }
	double ProcessNonpagedMemory()	{ return _processNonpagedMemoryValue.doubleValue; }
	double AvailableMemory()		{ return _availableMemoryValue.doubleValue; }
	double NonpagedMemory()			{ return _nonpagedMemoryValue.doubleValue; }
	long NetworkRecvMemory();
	long NetworkSendMemory();

private:

	//---------------------------------------------------------
	// CPU 사용율 관련 변수
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
	// 메모리 가용율 관련 변수
	//---------------------------------------------------------
	PDH_HCOUNTER   _processUserMemory;
	PDH_HCOUNTER   _processNonpagedMemory;
	PDH_HCOUNTER   _availableMemory;
	PDH_HCOUNTER   _nonpagedMemory;
	PDH_HCOUNTER   _networkRecvMemory;
	PDH_HCOUNTER   _networkSendMemory;

	PDH_HQUERY			 _cpuQuery;
	PDH_FMT_COUNTERVALUE _processUserMemoryValue;
	PDH_FMT_COUNTERVALUE _processNonpagedMemoryValue;
	PDH_FMT_COUNTERVALUE _availableMemoryValue;
	PDH_FMT_COUNTERVALUE _nonpagedMemoryValue;

	//---------------------------------------------------------
	// 네트워크 관련 변수 , 함수
	//---------------------------------------------------------
#define df_PDH_ETHERNET_MAX 8

	//--------------------------------------------------------------
	// 이더넷 하나에 대한 Send,Recv PDH 쿼리 정보.
	//--------------------------------------------------------------
	struct st_ETHERNET
	{
		bool _bUse;
		WCHAR _szName[128];
		PDH_HQUERY _recvQuery;
		PDH_HQUERY _sendQuery;
		PDH_HCOUNTER _pdh_Counter_Network_RecvBytes;
		PDH_HCOUNTER _pdh_Counter_Network_SendBytes;
	};
	st_ETHERNET _EthernetStruct[df_PDH_ETHERNET_MAX]; // 랜카드 별 PDH 정보
};

