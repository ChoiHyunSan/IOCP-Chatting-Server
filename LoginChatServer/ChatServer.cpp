#include "ChatServer.h"

MultiThreadChatServer g_server;

MultiThreadChatServer::MultiThreadChatServer()
{
	BOOL result = Initialize();
	if (result == false)
	{
		Log(L"ChatServer Init Error", 0, dfLOG_LEVEL_ERROR, L"ChatServer.txt");
		DebugBreak();
	}
}

MultiThreadChatServer::~MultiThreadChatServer()
{
	HANDLE hThreads[2];
	hThreads[0] = _hHeartBeatThread;
	hThreads[1] = _hMonitorThread;

	DWORD result = WaitForMultipleObjects(2, hThreads, TRUE, INFINITE);

	CloseHandle(_launchEvent);
	CloseHandle(_shutDownEvent);
	CloseHandle(_hHeartBeatThread);
	CloseHandle(_hMonitorThread);

	DeleteCriticalSection(&_userMapLock);
	for (int y = 0; y < dfSECTOR_SIZE; y++)
	{
		for (int x = 0; x < dfSECTOR_SIZE; x++)
		{
			DeleteCriticalSection(&_sectorLock[y][x]);
		}
	}

	for (int i = 0; i < _redisThreads.size(); i++)
	{
		if(_redisThreads[i].joinable())
			_redisThreads[i].join();
	}

	delete _userPool;
}

BOOL MultiThreadChatServer::OnConnectionRequest(const SOCKADDR_IN& sockAddr)
{
	return true;
}

VOID MultiThreadChatServer::OnClientJoin(const LONGLONG sessionID)
{
	AcceptUser(sessionID);
	InterlockedIncrement(&_updateTPS);
}

VOID MultiThreadChatServer::OnClientLeave(const LONGLONG sessionID)
{
	ReleaseUser(sessionID);
	InterlockedIncrement(&_updateTPS);
}

VOID MultiThreadChatServer::OnMessage(const LONGLONG sessionID, CPacket* packet)
{
	HandleClientJob(sessionID, packet);
	InterlockedIncrement(&_updateTPS);
}

VOID MultiThreadChatServer::OnError(const DWORD errCode, const WCHAR* str)
{
	return VOID();
}

VOID MultiThreadChatServer::OnLaunch()
{
	SetEvent(_launchEvent);
}

VOID MultiThreadChatServer::OnShutDown()
{
	SetEvent(_shutDownEvent);
}

BOOL MultiThreadChatServer::Initialize()
{
	// 쓰레드
	_hHeartBeatThread = (HANDLE)(HANDLE)_beginthreadex(NULL, 0, HeartBeatThread, this, 0, NULL);
	if (_hHeartBeatThread == NULL)
	{
		Log(L"ChatServer HeartBeatThread Init Error", 0, dfLOG_LEVEL_ERROR, L"ChatServer.txt");
		return false;
	}

	// MontorThread : Monitoring 전담 함수
	_hMonitorThread = (HANDLE)_beginthreadex(NULL, 0, MonitorThread, this, 0, NULL);
	if (_hMonitorThread == nullptr)
	{
		Log(L"Create Monitor Thread Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// 동기화 객체
	_launchEvent   = CreateEvent(NULL, TRUE, FALSE, NULL);
	_shutDownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	_userPool = new MemoryPool<User>(OBJECT_POOL_MIN);

	InitializeCriticalSection(&_userMapLock);
	for (int y = 0; y < dfSECTOR_SIZE; y++)
	{
		for (int x = 0; x < dfSECTOR_SIZE; x++)
		{
			InitializeCriticalSection(&_sectorLock[y][x]);
		}
	}	

	const int redisThreadCount = 1;
	_redisCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, redisThreadCount);
	for (int i = 0; i < redisThreadCount; i++)
	{
		_redisThreads.push_back(std::thread(RedisThread, this));
	}
	_redisClient.connect();
	return true;
}

UINT32 MultiThreadChatServer::HeartBeatThreadFunc()
{
	DWORD result = WaitForSingleObject(_launchEvent, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		Log(L"Launch Event Error", 0, dfLOG_LEVEL_ERROR, L"ChatServer.txt");
		return 0;
	}

	while (true)
	{
		DWORD result = WaitForSingleObject(_shutDownEvent, dfHEARTBEAT_LOOPTIME);
		if (result != WAIT_TIMEOUT)
		{
			if (result == WAIT_OBJECT_0)
			{
				Log(L"Terminate HeartBeatThread", 0, dfLOG_LEVEL_SYSTEM, L"ChatServer.txt");
				break;
			}
			else
			{
				Log(L"Events Error", 0, dfLOG_LEVEL_ERROR, L"ChatServer.txt");
				break;
			}
		}
		HeartbeatCheck();
	}

	return 0;
}

UINT32 MultiThreadChatServer::MonitorThreadFunc()
{
	for (;;)
	{
		DWORD result = WaitForSingleObject(_shutDownEvent, 1000);
		if (result != WAIT_TIMEOUT)
		{
			if (result != WAIT_OBJECT_0)
			{
				Log(L"WaitForSingleObject Error in ShutdownThread, reuslt : ", result, dfLOG_LEVEL_ERROR);
			}
			return 0;
		}

		// TPS 갱신
		DWORD recvTPS = _recvTPSCount;
		DWORD sendTPS = _sendTPSCount;
		DWORD acceptTPS = _acceptTPSCount;
		DWORD updateTPS = _updateTPS;
		DWORD userAllocCount    = _userPool->GetAllocCount();
		DWORD userCount			= _userCount;

		InterlockedExchange(&_recvTPSCount, 0);
		InterlockedExchange(&_sendTPSCount, 0);
		InterlockedExchange(&_acceptTPSCount, 0);
		InterlockedExchange(&_updateTPS, 0);

		// Monitor 객체 갱신
		_monitor.UpdateCpuTime();

#ifdef MONITOR
		system("cls");
		wcout << "-------------------------------------------" << endl;
		wcout << "              Chatting Server" << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Packet Total : " << CPacket::GetTotalSize() << endl;
		wcout << "Packet Alloc : " << CPacket::GetAllocCount() << endl;
		wcout << "Packet Free : " << CPacket::GetFreeCount() << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Session Count : " << _sessionCount << endl;
		wcout << "AcceptTotal : " << _acceptTotal << endl;
		wcout << "HeartBeat Disconnect : " << _heartbeatDisconnect << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "User Alloc Count : " << userAllocCount << endl;
		wcout << "User Count : " << userCount << endl;
		EnterCriticalSection(&_userMapLock);
		wcout << "User Map Size : " << _userMap.size() << endl;
		LeaveCriticalSection(&_userMapLock);
		wcout << "-------------------------------------------" << endl;
		wcout << "Send TPS : " << sendTPS << endl;
		wcout << "Recv TPS : " << recvTPS << endl;
		wcout << "Accept TPS : " << acceptTPS << endl;
		wcout << "Update TPS : " << updateTPS << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Processor CPU T : " << std::fixed << std::setprecision(2) << _monitor.ProcessorTotal() << " U : " << _monitor.ProcessorUser() << " K : " << _monitor.ProcessorKernel() << endl;
		wcout << "Process   CPU T : " << std::fixed << std::setprecision(2) << _monitor.ProcessTotal() << " U : " << _monitor.ProcessUser() << " K : " << _monitor.ProcessKernel() << endl;
		wcout << "Process   Mem U : " << std::fixed << std::setprecision(0) << _monitor.ProcessMemory() << " NP : " << _monitor.ProcessNonpagedMemory() << " (BYTE)" << endl;
		wcout << "Total Available Memory : " << std::fixed << std::setprecision(0) << _monitor.AvailableMemory() << " NP : " << _monitor.NonpagedMemory() << " (BYTE)" << endl;
#endif
	}

#ifdef DEBUG
	wcout << L"MonitorThread Terminate" << endl;
#endif
	return 0;
}

UINT32 MultiThreadChatServer::RedisThreadFunc()
{
	for (;;)
	{
		DWORD result = WaitForSingleObject(_shutDownEvent, 1000);
		if (result != WAIT_TIMEOUT)
		{
			if (result != WAIT_OBJECT_0)
			{
				Log(L"WaitForSingleObject Error in RedisThread, reuslt : ", result, dfLOG_LEVEL_ERROR);
			}
			return 0;
		}

		DWORD		cbTransferred = 0;
		LONGLONG	key = -1;
		OVERLAPPED* overlapped = nullptr;
		DWORD retval = GetQueuedCompletionStatus(_redisCP, &cbTransferred, (PULONG_PTR)&key, (LPOVERLAPPED*)&overlapped, INFINITE);
		if (cbTransferred == 0 && key == 0 && overlapped == 0) break;
		if (key == -1)
		{
			Log(L"Invalid User ptr Insert in Redis Completion Port", 0, dfLOG_LEVEL_ERROR);
			DebugBreak();
		}

		while (true)
		{
			LONGLONG sessionID;
			if (_redisQueue.Dequeue(sessionID) == false)
				break;

			EnterCriticalSection(&_userMapLock);
			User* user = _userMap[sessionID];
			if (user == nullptr)
			{
				_userMap.erase(sessionID);
				LeaveCriticalSection(&_userMapLock);
				continue;
			}
			EnterCriticalSection(&user->_lock);
			LeaveCriticalSection(&_userMapLock);

			HandleRequestID(user);

			LeaveCriticalSection(&user->_lock);
		}

	}

	return 0;
}

void MultiThreadChatServer::SendUnicast(CPacket* packet, const INT64 sessionID)
{
	packet->AddRefCount();
	if (SendPacket(sessionID, packet) == false)
	{
		CPacket::Free(packet);
	}
}

void MultiThreadChatServer::SendSector(CPacket* packet, const int sX, const int sY)
{
	EnterCriticalSection(&_sectorLock[sY][sX]);
	list<User*>::iterator iter = _sectorList[sY][sX].begin();
	for (; iter != _sectorList[sY][sX].end(); iter++)
	{
		SendUnicast(packet, (*iter)->_sessionID);
	}
	LeaveCriticalSection(&_sectorLock[sY][sX]);
}

void MultiThreadChatServer::SendAroundSector(CPacket* packet, User* user)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			int newX = (user->_sX + _dx[i]);
			int newY = (user->_sY + _dy[j]);

			if (newX < 0 || newY < 0 || newX >= dfSECTOR_SIZE || newY >= dfSECTOR_SIZE)
				continue;

			SendSector(packet, newX, newY);
		}
	}
}

inline void MultiThreadChatServer::HandleClientJob(const LONGLONG sessionID, CPacket* packet)
{
	EnterCriticalSection(&_userMapLock);
	User* user = _userMap[sessionID];
	if (user == nullptr)
	{
		_userMap.erase(sessionID);
		LeaveCriticalSection(&_userMapLock);
		return;
	}
	EnterCriticalSection(&user->_lock);
	LeaveCriticalSection(&_userMapLock);

	user->_prevHeartBeat = GetTickCount64();

	WORD type;
	(*packet) >> type;
	PacketHandler::HandlePacket(type, user, packet);

	LeaveCriticalSection(&user->_lock);
}

void MultiThreadChatServer::AcceptUser(const LONGLONG sessionID)
{
	EnterCriticalSection(&_userMapLock);
	User* user;
	if (_userMap.find(sessionID) != _userMap.end())
	{
		DebugBreak();
	}
	user = _userPool->Alloc();
	EnterCriticalSection(&user->_lock);

	user->Init(sessionID);
	_userMap[sessionID] = user;
	LeaveCriticalSection(&_userMapLock);
	InterlockedIncrement(&_userCount); 
	LeaveCriticalSection(&user->_lock);
}

VOID MultiThreadChatServer::ReleaseUser(const LONGLONG sessionID)
{
	EnterCriticalSection(&_userMapLock);
	User* oldUser = _userMap[sessionID];
	if (oldUser == nullptr)
	{
		_userMap.erase(sessionID);
		LeaveCriticalSection(&_userMapLock);
		return;
	}
	EnterCriticalSection(&oldUser->_lock);

	_userMap.erase(sessionID);
	LeaveCriticalSection(&_userMapLock);

	if (oldUser->_sX != 50)
		DeleteUserInCurrentSector(oldUser);

	InterlockedDecrement(&_userCount);

	oldUser->_activeFlag = false;
	oldUser->_sX = 50;
	oldUser->_sY = 50;

	LeaveCriticalSection(&oldUser->_lock);
	_userPool->Free(oldUser);
}

void MultiThreadChatServer::MoveUser(const WORD destX, const WORD destY, User* user)
{
	// 현재 위치에서 삭제
	DeleteUserInCurrentSector(user);

	user->_sX = destX;
	user->_sY = destY;

	// 새로운 위치에 추가 
	SetUserInSector(destX, destY, user);
}

void MultiThreadChatServer::SetUserInSector(const WORD sX, const WORD sY, User* user)
{
	EnterCriticalSection(&_sectorLock[sY][sX]);
	_sectorList[sY][sX].push_back(user);
	LeaveCriticalSection(&_sectorLock[sY][sX]);
}

void MultiThreadChatServer::DeleteUserInCurrentSector(User* user)
{
	int curX = user->_sX;
	int curY = user->_sY;
	if (curX >= 50 || curY >= 50)
		return;

	EnterCriticalSection(&_sectorLock[curY][curX]);
	list<User*>::iterator iter = _sectorList[curY][curX].begin();
	for (; iter != _sectorList[curY][curX].end(); iter++)
	{
		if ((*iter) == user)
		{
			_sectorList[curY][curX].erase(iter);
			break;
		}
	}
	LeaveCriticalSection(&_sectorLock[curY][curX]);
}

void MultiThreadChatServer::HeartbeatCheck()
{
	EnterCriticalSection(&_userMapLock);

	unordered_map<INT64, User*>::iterator iter = _userMap.begin();
	vector<INT64> disconnectArr;
	for (; iter != _userMap.end(); iter++)
	{
		if ((*iter).second == nullptr ||
			(*iter).second->_activeFlag == false)
		{
			continue;
		}
		if (GetTickCount64() - (*iter).second->_prevHeartBeat > dfHEARTBEAT_TIME)
		{
			disconnectArr.push_back((*iter).second->_sessionID);
		}
	}

	for (int id : disconnectArr)
	{
		InterlockedIncrement(&_heartbeatDisconnect);
		Disconnect(id);
	}

	LeaveCriticalSection(&_userMapLock);
}

void MultiThreadChatServer::HandleRequestID(User* user)
{
	// Redis 요청 꺼내기
	cpp_redis::reply reply = user->_redisFuture.get();
	const std::string& token = reply.as_string();
	BYTE status = (strncmp(token.c_str(), user->_sessionKey, 64) == 0)
		? dfGAME_LOGIN_OK
		: dfGAME_LOGIN_FAIL;

	CPacket* loginPacket = CPacket::Alloc();
	PacketHandler::CreatePacket_Login(loginPacket, status, user->_accountNo);
	SendUnicast(loginPacket, user->_sessionID);
	CPacket::Free(loginPacket);

	if (status == dfGAME_LOGIN_FAIL)
	{
		Disconnect(user->_sessionID);
	}
}

void MultiThreadChatServer::RequestCheckID(LONGLONG sessionID)
{
	// Redis Completion Port에 user에 대한 ID체크를 비동기적으로 처리하도록 요청한다.
	_redisQueue.Enqueue(sessionID);

	PostQueuedCompletionStatus(_redisCP, 0, 1, nullptr);
}

UINT32 __stdcall HeartBeatThread(VOID* arg)
{
	return reinterpret_cast<MultiThreadChatServer*>(arg)->HeartBeatThreadFunc();
}

UINT32 __stdcall MonitorThread(void* arg)
{
	return reinterpret_cast<MultiThreadChatServer*>(arg)->MonitorThreadFunc();
}

UINT32 __stdcall RedisThread(void* arg)
{
	return reinterpret_cast<MultiThreadChatServer*>(arg)->RedisThreadFunc();
}
