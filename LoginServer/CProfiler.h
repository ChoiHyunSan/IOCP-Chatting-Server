#pragma once
#include <Windows.h>
#include <unordered_map>
using namespace std;

#define DATA_COUNT 10
#define PROFILE

//////////////////////////////////////////////////////////////////////////////////
//
//	ProfilerManager
//	- 쓰레드마다 독립적인 Profiler를 동작할 수 있게 도와준다.
//	- PROFLIE 을 define하여 원할 때마다 사용할 수 있다.
// 
//////////////////////////////////////////////////////////////////////////////////
class CProfiler;
class ProfileManager
{
public:
	ProfileManager()
	{
		_profileIndex = TlsAlloc();
		if (_profileIndex == TLS_OUT_OF_INDEXES)
		{
			DebugBreak();
		}
		InitializeSRWLock(&_settingLock);
	}

public:
	void			Begin(const char* name);
	void			End(const char* name);
	void			Print(const char* fileName);
	void			Reset();
	CProfiler* GetTLSProfiler();

private:
	DWORD	_profileIndex;
	SRWLOCK _settingLock;
};

extern ProfileManager g_profileManager;
extern unordered_map<DWORD, CProfiler*> g_profilerHash;

#ifdef PROFILE
#define PRO_BEGIN(name)			g_profileManager.Begin(name)
#define PRO_END(name)			g_profileManager.End(name)
#define PRO_PRINT(fileName)		g_profileManager.Print(fileName)
#define PRO_RESET				g_profileManager.Reset()
#else
#define PRO_BEGIN(name)	
#define PRO_END(name)	
#define PRO_PRINT(fileName)
#define PRO_RESET
#endif

//////////////////////////////////////////////////////////////////////////////////
//
//	Profiler
//	- 원하는 기능에 대한 성능 분석을 위해 시간을 측정하여 진행한 결과를 보여준다.
//	- PROFLIE 을 define하여 원할 때마다 사용할 수 있다.
// 
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//
//	Profile_Data
//	- Profile에 필요한 데이터를 모아놓은 구조체
//	
//////////////////////////////////////////////////////////////////////////////////
struct Profile_Data
{
	bool			flag;				// 사용중인 샘플인지 체크
	char			name[64];			// 데이터 이름

	LARGE_INTEGER	startTime;			// 측정 시작 시간

	__int64			totalTime;			// 총합 시간
	__int64			min[2] = { 0,0 };	// 최소 값 
	__int64			max[2] = { 0,0 };	// 최대 값

	int				callCount;			// 호출 횟수

	Profile_Data() : flag(false), totalTime(0), callCount(0)
	{
		startTime.QuadPart = 0;

	}
};

class CProfiler
{
public:
	void Begin(const char* name);
	void End(const char* name);
	void Reset();
public:
	Profile_Data _profileDatas[DATA_COUNT];
	HANDLE	     _errorFIle;
};

