#include "CLoginServer.h"
#include "PacketHandler.h"

CLoginServer g_server;
CLoginServer::CLoginServer()
{
	BOOL result = Initialize();
	if (result == false)
	{
		Log(L"ChatServer Init Error", 0, dfLOG_LEVEL_ERROR, L"ChatServer.txt");
		DebugBreak();
	}
}

CLoginServer::~CLoginServer()
{
	// 쓰레드 종료 대기
	for (auto& thread : _threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	CloseHandle(_shutdownEvent);
	DeleteCriticalSection(&_userMapLock);
}

BOOL CLoginServer::OnConnectionRequest(const SOCKADDR_IN& sockAddr)
{
	return true;
}

VOID CLoginServer::OnClientJoin(const LONGLONG sessionID)
{
	// 로그인을 위해 유저 접속
	AcceptUser(sessionID);
}

VOID CLoginServer::OnClientLeave(const LONGLONG sessionID)
{
	// 유저연결 해제
	ReleaseUser(sessionID);
}

VOID CLoginServer::OnMessage(const LONGLONG sessionID, CPacket* packet)
{
	HandleClientJob(sessionID, packet);
}

VOID CLoginServer::OnError(const DWORD errCode, const WCHAR* str)
{

}

VOID CLoginServer::OnSend(const LONGLONG sessionID, CPacket* packet)
{
#ifdef DISCONNECT_BY_SERVER
	// Request에 대한 Response를 보냈으므로 서버측에서 연결을 끊는다.
	ReleaseUser(sessionID);
#endif
}

VOID CLoginServer::OnLaunch()
{

}

VOID CLoginServer::OnShutDown()
{
	SetEvent(_shutdownEvent);
}

void CLoginServer::MonitorFunc()
{
	while (true)
	{
		DWORD result = WaitForSingleObject(_shutdownEvent, 1000);
		if (result != WAIT_TIMEOUT)
		{
			if (result != WAIT_OBJECT_0)
			{
				Log(L"WaitForSingleObject Error in ShutdownThread, reuslt : ", result, dfLOG_LEVEL_ERROR);
			}
			return;
		}

		// Monitor 객체 갱신
		_monitor.UpdateCpuTime();

		// TPS 갱신
		DWORD recvTPS = _recvTPSCount;
		DWORD sendTPS = _sendTPSCount;
		DWORD acceptTPS = _acceptTPSCount;
		DWORD updateTPS = _updateTPS;
		DWORD dbTPS = _dbTPS;
		DWORD redisTPS = _redisTPS;
		DWORD userAllocCount = _userPool->GetAllocCount();
		DWORD userCount = _userCount;

		InterlockedExchange(&_recvTPSCount, 0);
		InterlockedExchange(&_sendTPSCount, 0);
		InterlockedExchange(&_acceptTPSCount, 0);
		InterlockedExchange(&_updateTPS, 0);
		InterlockedExchange(&_redisTPS, 0);
		InterlockedExchange(&_dbTPS, 0);

#ifdef MONITOR
		// system("cls");
		wcout << "-------------------------------------------" << endl;
		wcout << "               Login Server" << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Packet Total : " << CPacket::GetTotalSize() << endl;
		wcout << "Packet Alloc : " << CPacket::GetAllocCount() << endl;
		wcout << "Packet Free : " << CPacket::GetFreeCount() << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Session Count : " << _sessionCount << endl;
		wcout << "AcceptTotal : " << _acceptTotal << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "User Alloc Count : " << userAllocCount << endl;
		wcout << "User Count : " << userCount << endl;
		wcout << "User Map Size : " << _userMap.size() << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Send TPS : " << sendTPS << endl;
		wcout << "Recv TPS : " << recvTPS << endl;
		wcout << "Accept TPS : " << acceptTPS << endl;
		wcout << "Update TPS : " << updateTPS << endl;
		wcout << "DB TPS : " << dbTPS << " Queue Size : " << _dbQueue.GetSize() << endl;
		wcout << "Redis TPS : " << redisTPS << " Queue Size : " << _redisQueue.GetSize() << endl;
		wcout << "Heartbeat Count : " << _heartbeatCnt << endl;
		wcout << "-------------------------------------------" << endl;
		wcout << "Processor CPU T : " << std::fixed << std::setprecision(2) << _monitor.ProcessorTotal() << " U : " << _monitor.ProcessorUser() << " K : " << _monitor.ProcessorKernel() << endl;
		wcout << "Process   CPU T : " << std::fixed << std::setprecision(2) << _monitor.ProcessTotal() << " U : " << _monitor.ProcessUser() << " K : " << _monitor.ProcessKernel() << endl;
		wcout << "Process   Mem U : " << std::fixed << std::setprecision(0) << _monitor.ProcessMemory() << " (BYTE)" << " NP : " << _monitor.ProcessNonpagedMemory() << " (BYTE)" <<endl;
		wcout << "Total Available Memory : " << std::fixed << std::setprecision(0) << _monitor.AvailableMemory() << " (MB)" << " NP : " << _monitor.NonpagedMemory() << " (BYTE)" << endl;
#endif
	}

#ifdef DEBUG
	wcout << L"MonitorThread Terminate" << endl;
#endif
}


void CLoginServer::HeartbeatFunc()
{
	while (true)
	{
		EnterCriticalSection(&_userMapLock);

		unordered_map<INT64, User*>::iterator iter = _userMap.begin();
		vector<INT64> disconnectArr;
		for (; iter != _userMap.end(); iter++)
		{
			if ((*iter).second == nullptr) continue;

			if (GetTickCount64() - (*iter).second->_prevHeartBeat > dfHEARTBEAT_TIME)
			{
				disconnectArr.push_back((*iter).second->_sessionID);
			}
		}

		for (LONGLONG id : disconnectArr)
		{
			if (Disconnect(id))
			{
				ReleaseUser(id);
				InterlockedIncrement(&_heartbeatCnt);
			}
		}

		LeaveCriticalSection(&_userMapLock);
	}
}

bool CLoginServer::Initialize()
{
	// 동기화 객체
	_shutdownEvent = CreateEvent(nullptr, true, false, nullptr);

	_userPool = new MemoryPool<User>(OBJECT_POOL_MIN);
	InitializeCriticalSection(&_userMapLock);
	
	_dbEvent = CreateEvent(nullptr, false, false, nullptr);
	_redisEvent = CreateEvent(nullptr, false, false, nullptr);

	// 쓰레드 생성
	_threads.push_back(std::thread(MonitorThreadFunc, this));
	_threads.push_back(std::thread(HeartBeatThreadFunc, this));
	_threads.push_back(std::thread(DBThreadFunc, this));
	_threads.push_back(std::thread(RedisThreadFunc, this));
	_threads.push_back(std::thread(RedisThreadFunc, this));
	_threads.push_back(std::thread(RedisThreadFunc, this));

	_redisClient.connect();

	// DB 세팅
	CParsor parsor;
	BOOL result = parsor.LoadFile("DBSetting.txt");
	if (result == false)
	{
		Log(L"Load DBSetting file failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}
	if (!parsor.GetValue("SERVER_IP", (int*)_serverIP, sizeof(_serverIP))
		|| !parsor.GetValue("USER", (int*)_user, sizeof(_user))
		|| !parsor.GetValue("PASSWORD", (int*)_password, sizeof(_password))
		|| !parsor.GetValue("TABLE", (int*)_table, sizeof(_table))
		|| !parsor.GetValue("PORT", &_port))
	{
		Log(L"Set DB value failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}
	_playerDB = new SQL(_serverIP, _user, _password, _table, _port);


	return true;
}

void CLoginServer::SendUnicast(CPacket* packet, const INT64 sessionID)
{
	packet->AddRefCount();
	if (SendPacket(sessionID, packet) == false)
	{
		CPacket::Free(packet);
	}
}

void CLoginServer::DBFunc()
{
	HANDLE events[2];
	events[0] = _shutdownEvent;
	events[1] = _dbEvent;

	for (;;)
	{
		DWORD result = WaitForMultipleObjects(2, events, false, INFINITE);
		if (result != WAIT_OBJECT_0 + 1)
		{
			if (result == WAIT_OBJECT_0)
			{
				// Shutdown
				break;
			}
			else {}
		}
		
		for (;;)
		{
			LONGLONG sessionID;
			if (_dbQueue.Dequeue(sessionID) == false)
				break;

			User* user = _userMap[sessionID];
			if (user == nullptr)
			{
				_userMap.erase(sessionID);
				continue;
			}
			EnterCriticalSection(&user->_lock);

			// DB에 쿼리를 날려서 값을 받아오는 과정을 가정
			char query[100];
			snprintf(query, sizeof(query), "select userpass from account where accountno = %I64d;", user->_accountNo);
			LeaveCriticalSection(&user->_lock);
			if (_playerDB->ExecQuery(query) == nullptr)
			{
				DebugBreak();
			}

			// 인증을 마친 후, Redis 쓰레드에 일감 넘기기
			_redisQueue.Enqueue(sessionID);
			SetEvent(_redisEvent);
			InterlockedIncrement(&_dbTPS);
			continue;
		}
	}
}

void CLoginServer::RedisFunc()
{
	HANDLE events[2];
	events[0] = _shutdownEvent;
	events[1] = _redisEvent;

	for (;;)
	{
		DWORD result = WaitForMultipleObjects(2, events, false, INFINITE);
		if (result != WAIT_OBJECT_0 + 1)
		{
			if (result == WAIT_OBJECT_0)
			{
				// Shutdown
				break;
			}
			else {}
		}

		for (;;)
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

			g_server._redisClient.set(to_string(user->_accountNo), string(user->_sessionKey));
			g_server._redisClient.sync_commit();

			WCHAR  GameServer[16] = L"127.0.0.1";
			USHORT GameServerPort = 10000;
			WCHAR  ChatServer[16] = L"127.0.0.1";
			USHORT ChatServerPort = 6000;

			// Step 2. Response Packet
			CPacket* loginPacket = CPacket::Alloc();
			PacketHandler::CreatePacket_Login(
				loginPacket,
				en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK,
				user->_accountNo,
				reinterpret_cast<char*>(user->_ID),
				reinterpret_cast<char*>(user->_NickName),
				reinterpret_cast<char*>(GameServer),
				GameServerPort,
				reinterpret_cast<char*>(ChatServer),
				ChatServerPort);

			g_server.SendUnicast(loginPacket, user->_sessionID);
			CPacket::Free(loginPacket);
			LeaveCriticalSection(&user->_lock);

			InterlockedIncrement(&_redisTPS);
		}
	}
}

inline void CLoginServer::HandleClientJob(const LONGLONG sessionID, CPacket* packet)
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

void CLoginServer::AcceptUser(const LONGLONG sessionID)
{
	EnterCriticalSection(&_userMapLock);
	User* user;
	if (_userMap.find(sessionID) != _userMap.end())
	{
		LeaveCriticalSection(&_userMapLock);
		return;
	}
	user = _userPool->Alloc();
	EnterCriticalSection(&user->_lock);

	user->Init(sessionID);
	_userMap[sessionID] = user;
	LeaveCriticalSection(&_userMapLock);
	InterlockedIncrement(&_userCount);
	LeaveCriticalSection(&user->_lock);
}

void CLoginServer::ReleaseUser(const LONGLONG sessionID)
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

	InterlockedDecrement(&_userCount);

	LeaveCriticalSection(&oldUser->_lock);
	_userPool->Free(oldUser);
}

void MonitorThreadFunc(CLoginServer* arg)
{
	arg->MonitorFunc();
}

void HeartBeatThreadFunc(CLoginServer* arg)
{
	arg->HeartbeatFunc();
}

void DBThreadFunc(CLoginServer* arg)
{
	arg->DBFunc();
}

void RedisThreadFunc(CLoginServer* arg)
{
	arg->RedisFunc();
}
