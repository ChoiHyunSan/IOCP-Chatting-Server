#pragma once
#include <Windows.h>
#include <unordered_map>
using namespace std;

#define DATA_COUNT 10
#define PROFILE

//////////////////////////////////////////////////////////////////////////////////
//
//	ProfilerManager
//	- �����帶�� �������� Profiler�� ������ �� �ְ� �����ش�.
//	- PROFLIE �� define�Ͽ� ���� ������ ����� �� �ִ�.
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
//	- ���ϴ� ��ɿ� ���� ���� �м��� ���� �ð��� �����Ͽ� ������ ����� �����ش�.
//	- PROFLIE �� define�Ͽ� ���� ������ ����� �� �ִ�.
// 
//////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////
//
//	Profile_Data
//	- Profile�� �ʿ��� �����͸� ��Ƴ��� ����ü
//	
//////////////////////////////////////////////////////////////////////////////////
struct Profile_Data
{
	bool			flag;				// ������� �������� üũ
	char			name[64];			// ������ �̸�

	LARGE_INTEGER	startTime;			// ���� ���� �ð�

	__int64			totalTime;			// ���� �ð�
	__int64			min[2] = { 0,0 };	// �ּ� �� 
	__int64			max[2] = { 0,0 };	// �ִ� ��

	int				callCount;			// ȣ�� Ƚ��

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

