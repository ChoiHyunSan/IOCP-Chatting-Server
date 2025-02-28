#include "CProfiler.h"
#include <iostream>

ProfileManager g_profileManager;
unordered_map<DWORD, CProfiler*> g_profilerHash;

//////////////////////////////////////////////////////////////////////////////////
//
//  Begin(const char* name)
//	- ���� Ÿ�̸� üũ�� �����ϴ� �Լ�
//	- ���� ������ �ִ� ������ �ʰ��ϰų� ¦�� ���� �ʰ� ȣ��Ǵ� ��� ������ �α��Ѵ�.
// 
//////////////////////////////////////////////////////////////////////////////////
void CProfiler::Begin(const char* name)
{
	// �̹� �ִ� ���
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true
			&& strcmp(_profileDatas[index].name, name) == 0)
		{
			// �ʱ�ȭ �Ǿ����� ���� ��� (End�� ȣ����� �ʰ� �ٽ� Begin�� ȣ��� ���)
			if (_profileDatas[index].startTime.QuadPart != 0)
			{
				return;
			}

			strcpy_s(_profileDatas[index].name, name);
			_profileDatas[index].flag = true;
			_profileDatas[index].callCount += 1;
			QueryPerformanceCounter(&_profileDatas[index].startTime);
			return;
		}
	}

	// ���� ���
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == false)
		{
			if (_profileDatas[index].startTime.QuadPart != 0)
			{
				return;
			}

			strcpy_s(_profileDatas[index].name, name);
			_profileDatas[index].flag = true;
			_profileDatas[index].callCount += 1;
			QueryPerformanceCounter(&_profileDatas[index].startTime);
			return;
		}
	}

	// �� �̻� �±׸� �� �� ���� ��� (out of data_count)
	return;
}

void CProfiler::End(const char* name)
{
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true
			&& strcmp(_profileDatas[index].name, name) == 0)
		{
			// �ð��� �����Ǿ� ���� ���� ���
			if (_profileDatas[index].startTime.QuadPart == 0)
			{
				return;
			}
			LARGE_INTEGER endTime;
			QueryPerformanceCounter(&endTime);

			_int64 time = endTime.QuadPart - _profileDatas[index].startTime.QuadPart;

			if (_profileDatas[index].max[0] < time)
			{
				if (_profileDatas[index].max[1] < time)
				{
					_profileDatas[index].max[0] = _profileDatas[index].max[1];
					_profileDatas[index].max[1] = time;
				}
				else
				{
					_profileDatas[index].max[0] = time;
				}
			}
			else if (_profileDatas[index].min[0] > time
				|| _profileDatas[index].min[0] == 0)
			{
				if (_profileDatas[index].min[1] > time
					|| _profileDatas[index].min[1] == 0)
				{
					_profileDatas[index].min[0] = _profileDatas[index].min[1];
					_profileDatas[index].min[1] = time;
				}
				else
				{
					_profileDatas[index].min[0] = time;
				}
			}

			_profileDatas[index].totalTime += time;
			_profileDatas[index].startTime.QuadPart = 0;

			return;
		}
	}

	// �߸� End�� ȣ��� ���

}

void CProfiler::Reset()
{
	for (int index = 0; index < DATA_COUNT; index++)
	{
		if (_profileDatas[index].flag == true)
		{
			_profileDatas[index].callCount = 0;
			_profileDatas[index].max[0] = 0;
			_profileDatas[index].max[1] = 0;
			_profileDatas[index].min[0] = 0;
			_profileDatas[index].min[1] = 0;
			_profileDatas[index].startTime.QuadPart = 0;
			_profileDatas[index].totalTime = 0;
		}
	}
}

void ProfileManager::Begin(const char* name)
{
	GetTLSProfiler()->Begin(name);
}

void ProfileManager::End(const char* name)
{
	GetTLSProfiler()->End(name);
}

void ProfileManager::Print(const char* fileName)
{
	FILE* pFile;
	fopen_s(&pFile, fileName, "w");
	if (pFile == nullptr)
	{
		return;
	}

	for (unordered_map<DWORD, CProfiler*>::iterator iter = g_profilerHash.begin(); iter != g_profilerHash.end(); iter++)
	{
		DWORD		threadID = iter->first;
		CProfiler* profiler = iter->second;

		fprintf(pFile, "-------------------------------------------------------------------------------------------------------- \n");
		fprintf(pFile, "|     ThreadID    |                  Name |            Average |         Min |         Max |       Call | \n");

		for (int index = 0; index < DATA_COUNT; index++)
		{
			Profile_Data* profileData = &profiler->_profileDatas[index];
			if (profileData->flag == false) continue;

			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);

			// ���, Min, Max ���ϱ�
			int minMaxCount = 0;
			if (profileData->max[1] != 0) minMaxCount++;
			if (profileData->max[0] != 0) minMaxCount++;
			if (profileData->min[0] != 0) minMaxCount++;
			if (profileData->min[1] != 0) minMaxCount++;

			__int64 totalCount = profileData->totalTime - (profileData->max[0] + profileData->max[1] + profileData->min[0] + profileData->min[1]);
			float average = (float)totalCount / (profileData->callCount - minMaxCount) / freq.QuadPart * 1000;
			float min = (float)profileData->min[1] / freq.QuadPart * 1000;
			float max = (float)profileData->max[1] / freq.QuadPart * 1000;

			fprintf(pFile, "|     %12d |%24s |%17.4fms |%10.4f |%9.4fms |%11d | \n", threadID, profileData->name, average, min, max, profileData->callCount);
		}
		fprintf(pFile, "-------------------------------------------------------------------------------------------------------- \n");
	}
	fclose(pFile);
}

void ProfileManager::Reset()
{
	for (unordered_map<DWORD, CProfiler*>::iterator iter = g_profilerHash.begin(); iter != g_profilerHash.end(); iter++)
	{
		CProfiler* profiler = iter->second;
		if (profiler)
		{
			profiler->Reset();
		}
	}
}

inline CProfiler* ProfileManager::GetTLSProfiler()
{
	CProfiler* profiler = static_cast<CProfiler*>(TlsGetValue(_profileIndex));
	if (profiler == nullptr)
	{
		profiler = new CProfiler;

		TlsSetValue(_profileIndex, static_cast<LPVOID>(profiler));

		AcquireSRWLockExclusive(&_settingLock);
		g_profilerHash[GetCurrentThreadId()] = profiler;
		ReleaseSRWLockExclusive(&_settingLock);
	}

	return profiler;
}
