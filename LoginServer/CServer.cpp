#include "CServer.h"

class Session;

CServer::CServer()
{
	_sessionPool = new MemoryPool<Session>(OBJECT_POOL_MIN, false);

	BOOL result = ServerSetting();
	if (result == false)
	{
		Log(L"Server Setting Fail", 0, dfLOG_LEVEL_ERROR);
	}
}
CServer::~CServer()
{
	BOOL result = ReleaseServer();
	if (result == false)
	{
		Log(L"Release Server Fail", 0, dfLOG_LEVEL_ERROR);
	}
}

// TODO : 입력과 모니터링 기능 밖으로 빼기 -> 인터페이스화 하기
BOOL CServer::Launch()
{
	// listen()
	BOOL result = listen(_listenSocket, SOMAXCONN);
	if (result == SOCKET_ERROR)
	{
		Log(L"Listen Func Error", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	LaunchWorkerThread();
	OnLaunch();

	char ch;
	for (;;)
	{
		result = WaitForSingleObject(_shutdownEvent, 0);
		if (result == WAIT_OBJECT_0)
		{
#ifdef DEBUG
			wcout << L"Exit Main Loop" << endl;
#endif
			break;
		}

		// Remote Server
		if (_kbhit())
		{
			ch = _getch();
			switch (ch)
			{
			case 'Q':
			case 'q':
			{
				ShutDown();
				break;
			}
			case 'R':
			case 'r':
			{
				break;
			}
			case 'S':
			case 's':
			{
				PRO_PRINT("CServer_Profiler.txt");
			}
			}
		}
	}

	return true;
}

VOID CServer::ShutDown()
{
	SetEvent(_shutdownEvent);
}

DWORD CServer::GetSessionCount()
{
	return _sessionCount;
}

DWORD CServer::GetAcceptTPS()
{
	return _acceptTPSCount;
}

DWORD CServer::GetRecvMessageTPS()
{
	return _recvTPSCount;
}

DWORD CServer::GetSendMessageTPS()
{
	return _sendTPSCount;
}

BOOL CServer::ServerSetting()
{
	// Parse ServerInfo
	BOOL result = ParsingServerInfo();
	if (result == false)
	{
		Log(L"Parsing ServerInfo Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	result = SocketAndCPSetting();
	if (result == false)
	{
		Log(L"Socket Programming Setting Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	InitializeSRWLock(&_emptyIndexLock);

	for (int cnt = 0; cnt < _sessionMaxCount; cnt++)
	{
		_emptyIndexQueue.push(cnt);
	}

	_sessionArray = new Session * [_sessionMaxCount];
	memset(_sessionArray, 0, _sessionMaxCount * sizeof(Session*));

	_shutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_launchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	result = ThreadSetting();
	if (result == false)
	{
		Log(L"Thread Setting Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	return true;
}

BOOL CServer::ParsingServerInfo()
{
	WParsor parsor;
	BOOL result = parsor.LoadFile(L"ServerSetting.txt");
	if (result == false)
	{
		Log(L"LoadFile Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// 세팅 값 가져와서 설정
	if (!parsor.GetValue(SERVER_PORT, (int*)&_serverPort)
		|| !parsor.GetValue(BUFFER_SIZE, (int*)&_bufferSize)
		|| !parsor.GetValue(WORKER_THREAD_COUNT, (int*)&_workerThreadCount)
		|| !parsor.GetValue(ACTIVE_THREAD_COUNT, (int*)&_activeThreadCount)
		|| !parsor.GetValue(SESSION_MAX, (int*)&_sessionMaxCount)
		|| !parsor.GetValue(USER_MAX, (int*)&_userMaxCount)
		|| !parsor.GetValue(SERVER_IP, (int*)_serverIP, sizeof(_serverIP))
		|| !parsor.GetValue(PACKET_CODE, (int*)&_packetCode)
		|| !parsor.GetValue(PACKET_KEY, (int*)&_packetKey)
		|| !parsor.GetValue(NAGLE, (int*)&_nagleflag)
		|| !parsor.GetValue(SERVER_TYPE, (int*)&_serverType, sizeof(_serverType)))
	{
		Log(L"Invalid File Value Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// 최소한의 동작이 가능한 크기 범위를 지키는지 확인
	if (_bufferSize < 0 || _serverPort < 0 || _workerThreadCount < 1 || _sessionMaxCount < 1 || _userMaxCount < 1)
	{
		Log(L"Value boundary Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	return true;
}

BOOL CServer::SocketAndCPSetting()
{
	WSAData wsaData;
	BOOL result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		Log(L"WSAStartup Error", 0, dfLOG_LEVEL_ERROR);
		wcout << L"WSAStartup Error" << endl;
		return false;
	}

	// 입출력 완료 포트 생성
	_iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, _activeThreadCount);
	if (_iocpHandle == NULL)
	{
		Log(L"Create CP Error", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// socket()
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_listenSocket == INVALID_SOCKET)
	{
		Log(L"Create ListenSocket Error", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	int sendBufSize = 0;
	::setsockopt(_listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufSize, sizeof(sendBufSize));

	LINGER linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;
	if (SOCKET_ERROR == setsockopt(_listenSocket, SOL_SOCKET, SO_LINGER, (char*)&linger, sizeof(linger)))
	{
		Log(L"Set socket option Error", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(_serverPort);
	result = ::bind(_listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
	if (result == SOCKET_ERROR)
	{
		Log(L"Bind Func Error", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	return true;
}

BOOL CServer::ThreadSetting()
{
	_workerThreads = new HANDLE[_workerThreadCount];

	// ShutdownThread 
	_shutdownThread = (HANDLE)_beginthreadex(NULL, 0, ShutDownThread, this, 0, NULL);
	if (_shutdownThread == nullptr)
	{
		Log(L"Create ShutDown Thread Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	// WorkerThread : Accpet Thread를 제외한 개수
	for (int i = 0; i < _workerThreadCount; i++) {
		_workerThreads[i] = (HANDLE)_beginthreadex(NULL, 0, WorkerThread, this, 0, NULL);
		if (_workerThreads[i] == NULL)
		{
			Log(L"Create Worker Thread Failed", 0, dfLOG_LEVEL_ERROR);
			return false;
		}
	}

	// AcceptThread : Accept 전담 함수
	_acceptThread = (HANDLE)_beginthreadex(NULL, 0, AcceptThread, this, 0, NULL);
	if (_acceptThread == nullptr)
	{
		Log(L"Create Accept Thread Failed", 0, dfLOG_LEVEL_ERROR);
		return false;
	}

	return true;
}

BOOL CServer::ReleaseServer()
{
	// 생성한 쓰레드들을 기다린다.
	WaitForSingleObject(_shutdownThread, INFINITE);
	WaitForMultipleObjects(_workerThreadCount, _workerThreads, TRUE, INFINITE);

	closesocket(_listenSocket);
	WaitForSingleObject(_acceptThread, INFINITE);

	// 쓰레드 리소스 반환
	CloseHandle(_shutdownThread);
	CloseHandle(_acceptThread);
	for (int i = 0; i < _workerThreadCount; i++)
	{
		if (_workerThreads[i] == NULL)
		{
			Log(L"Thread Nullptr Error", 0, dfLOG_LEVEL_ERROR);
			return false;
		}

		CloseHandle(_workerThreads[i]);
	}


	delete[]	_workerThreads;
	delete		_sessionPool;
	delete[]    _sessionArray;

	WSACleanup();
	return true;
}

BOOL CServer::Disconnect(const LONGLONG sessionID)
{
	Session* session = AccessSession(sessionID);
	if (session == nullptr)
	{
		return false;
	}

	// 더이상 작업이 이뤄지지 않게 모든 IO를 중단시킨다.
	CancelIoEx((HANDLE)session->_socket, &session->_recvOverlapped);
	CancelIoEx((HANDLE)session->_socket, &session->_sendOverlapped);
	DeaccessSession(session);

	static int cnt = 0;
	cnt++;

	if (session->_RefCount == 0)
	{
		ReleaseSession(session, session->_ID);
	}
	return true;
}

BOOL CServer::SendPacket(const LONGLONG sessionID, CPacket* packet)
{
	Session* session = AccessSession(sessionID);
	if (session == nullptr)
	{
		return false;
	}

	// 네트워크 헤더 씌우기
	switch (_serverType)
	{
	case en_SERVER_TYPE::LAN:
		AddHeaderByLanVersion(packet);
		break;

	case en_SERVER_TYPE::NET:
		AddHeaderByNetVersion(packet);
		break;
	}

	InterlockedIncrement(&_sendTPSCount);
	session->_sendQ.Enqueue(packet);

	if (session->_ID != sessionID)
	{
		DebugBreak();
	}

	SendPost(session);
	DeaccessSession(session);

	return true;
}

VOID CServer::ReserveDisconnectSession(const LONGLONG sessionID, CPacket* packet)
{
	Session* session = AccessSession(sessionID);
	if (session == nullptr)
	{
		return;
	}
	session->_reserveDisconnect = true;
	session->_lastPacket = packet;
	DeaccessSession(session);
}


VOID CServer::HandleSend(Session* session, const DWORD sendBytes)
{
	if (WaitForSingleObject(_shutdownEvent, 0) == WAIT_OBJECT_0)
	{
		return;
	}

	PRO_BEGIN("HandleSend");

#ifdef DEBUG
	wcout << L"Send Success, SendBytes : " << sendBytes << endl;
#endif
	if (session->_lastPacket)
	{
		for (int i = 0; i < session->_wsaBufCount; i++)
		{
			CPacket* packet = session->_wsaPacket[i];
			if (packet == nullptr)
				break;

			if (packet == session->_lastPacket)
			{
				Disconnect(session->_ID);
			}

			CPacket::Free(session->_wsaPacket[i]);
			session->_wsaPacket[i] = nullptr;
		}
	}
	else
	{
		for (int i = 0; i < session->_wsaBufCount; i++)
		{
			CPacket* packet = session->_wsaPacket[i];
			if (packet == nullptr)
				break;

			CPacket::Free(session->_wsaPacket[i]);
			session->_wsaPacket[i] = nullptr;
		}
	}

	session->_wsaBufCount = 0;
	InterlockedExchange(&session->_sendCnt, 0);

	DWORD useSize = session->_sendQ.GetSize();
	if (useSize > 0)
	{
		SendPost(session);
	}

	PRO_END("HandleSend");
	return;
}

VOID CServer::HandleRecv(Session* session, const DWORD recvBytes)
{
	if (WaitForSingleObject(_shutdownEvent, 0) == WAIT_OBJECT_0)
	{
		return;
	}

	PRO_BEGIN("HandleRecv");

	DWORD moveSize = session->_recvRingBuffer.MoveRear(recvBytes);
	if (moveSize != recvBytes)
	{
		Log(L"WSARecv Error", 0, dfLOG_LEVEL_ERROR);
		DebugBreak();
	}

	SendMsgToContents(session);
	RecvPost(session);
	PRO_END("HandleRecv");
}

VOID CServer::SendMsgToContents(Session* session)
{
	switch (_serverType)
	{
	case en_SERVER_TYPE::LAN:
		HandleMsgByLanVersion(session);
		break;

	case en_SERVER_TYPE::NET:
		HandleMsgByNetVersion(session);
		break;
	}
}

VOID CServer::HandleMsgByLanVersion(Session* session)
{
	for (;;)
	{
		DWORD useSize = session->_recvRingBuffer.GetUseSize();
		if (useSize <= sizeof(PacketHeader))
		{
			break;
		}

		PacketHeader header;
		DWORD peekSize = session->_recvRingBuffer.Peek((char*)&header, sizeof(header));
		DWORD totalSize = sizeof(header) + header._len;
		if (useSize < totalSize)
		{
			break;
		}
		session->_recvRingBuffer.MoveFront(sizeof(header));

		// 헤더를 떼어내어 콜백함수 호출
		CPacket* contentsPacket = CPacket::Alloc();
		DWORD dequeueSize = session->_recvRingBuffer.Dequeue(contentsPacket->GetBufferPtr(), header._len);
		if (dequeueSize != header._len)
		{
			DebugBreak();
			return;
		}

		contentsPacket->MoveWritePos(header._len);
		InterlockedIncrement(&_recvTPSCount);
		OnMessage(session->_ID, contentsPacket);

		CPacket::Free(contentsPacket);
	}
}

VOID CServer::HandleMsgByNetVersion(Session* session)
{
	for (;;)
	{
		PRO_BEGIN("HandleMsg");

		DWORD useSize = session->_recvRingBuffer.GetUseSize();
		if (useSize <= sizeof(NetPacketHeader))
		{
			PRO_END("HandleMsg");
			break;
		}

		NetPacketHeader header;
		DWORD peekSize = session->_recvRingBuffer.Peek((char*)&header, sizeof(header));

		// 패킷코드 확인
		if (header._code != _packetCode)
		{
			Disconnect(session->_ID);
			PRO_END("HandleMsg");
			break;
		}

		// 총 길이 : 헤더 + 페이로드
		DWORD totalSize = sizeof(header) + header._len;
		if (useSize < totalSize)
		{
			PRO_END("HandleMsg");
			break;
		}
		// 체크섬 제외하고 빼기
		session->_recvRingBuffer.MoveFront(sizeof(header) - sizeof(BYTE));

		CPacket* contentsPacket = CPacket::Alloc();
		DWORD dequeueSize = session->_recvRingBuffer.Dequeue(contentsPacket->GetBufferPtr(), header._len + sizeof(BYTE));
		if (dequeueSize != header._len + sizeof(BYTE))
		{
			DebugBreak();
		}
		BYTE* checksum = (BYTE*)(contentsPacket->GetBufferPtr());

		// [체크섬] [페이로드] Decoding
		CEncryptionHelper::DecodePacket((BYTE*)contentsPacket->GetBufferPtr(), header._len + sizeof(BYTE), _packetKey, header._randKey);

		// 체크섬 계산
		BYTE calChecksum = CEncryptionHelper::GetCheckSum((BYTE*)contentsPacket->GetBufferPtr() + sizeof(BYTE), header._len);
		if (*checksum != calChecksum)
		{
			// 체크섬 일치 X
			CPacket::Free(contentsPacket);
			Disconnect(session->_ID);
			PRO_END("HandleMsg");
			return;
		}

		contentsPacket->MoveReadPos(sizeof(BYTE));
		contentsPacket->MoveWritePos(header._len + sizeof(BYTE));
		InterlockedIncrement(&_recvTPSCount);
		OnMessage(session->_ID, contentsPacket);

		CPacket::Free(contentsPacket);
		PRO_END("HandleMsg");
	}
}

VOID CServer::AddHeaderByLanVersion(CPacket* packet)
{
	// 헤더 제작
	PacketHeader header;
	header._len = packet->GetDataSize();

	// 패킷에 헤더 추가
	packet->AddHeader(&header, sizeof(PacketHeader));
}

VOID CServer::AddHeaderByNetVersion(CPacket* packet)
{
	if (packet->HeaderCheck()) return;

	// 헤더 제작
	NetPacketHeader header;
	header._len = packet->GetDataSize();
	header._code = _packetCode;
	header._randKey = (BYTE)(rand() % 256);
	header._checksum = CEncryptionHelper::GetCheckSum((BYTE*)packet->GetBufferPtr(), packet->GetDataSize());

	// 패킷에 헤더 추가
	packet->AddHeader(&header, sizeof(NetPacketHeader));

	// 패킷 인코딩
	CEncryptionHelper::EncodePacket((BYTE*)packet->GetBufferPtr() + 4, packet->GetDataSize(), _packetKey, header._randKey);
}

VOID CServer::RecvPost(Session* session)
{
	DWORD flags = 0;
	DWORD recvBytes = 0;

	IncrementRefCount(session);

	::memset(&session->_recvOverlapped, 0, sizeof(OVERLAPPED));
	int directEnqueueSize = session->_recvRingBuffer.DirectEnqueueSize();
	session->_wsaRecvBuf[0].buf = session->_recvRingBuffer.GetNextIndex();
	session->_wsaRecvBuf[0].len = directEnqueueSize;
	session->_wsaRecvBuf[1].buf = session->_recvRingBuffer.GetBufferPtr();
	session->_wsaRecvBuf[1].len = session->_recvRingBuffer.GetFreeSize() - directEnqueueSize;

	int result = WSARecv(session->_socket, session->_wsaRecvBuf, 2, &recvBytes,
		&flags, &session->_recvOverlapped, NULL);

	if (result != 0)
	{
		int errCode = WSAGetLastError();
		if (errCode != ERROR_IO_PENDING)
		{
			if (errCode != 10054 && errCode != 10053)
			{
				Log(L"WSARecv Error", 0, dfLOG_LEVEL_ERROR);
			}
			DecrementRefCount(session);
		}
	}
	return;
}

// Return : WSASend 여부
BOOL CServer::SendPost(Session* session)
{
	if (session->_sendQ.GetSize() <= 0)
	{
		return false;
	}

	if (InterlockedExchange(&session->_sendCnt, 1) == 1)
	{
		return false;
	}

	DWORD flags = 0;
	DWORD sendBytes = 0;

	IncrementRefCount(session);

	::memset(session->_wsaPacket, 0, sizeof(session->_wsaPacket));
	session->_wsaBufCount = 0;
	::memset(&session->_sendOverlapped, 0, sizeof(OVERLAPPED));

	for (;;)
	{
		if (session->_sendQ.Dequeue(session->_wsaPacket[session->_wsaBufCount]) == false)
		{
			break;
		}

		session->_wsaSendBuf[session->_wsaBufCount].buf = session->_wsaPacket[session->_wsaBufCount]->GetBufferPtr();
		session->_wsaSendBuf[session->_wsaBufCount].len = session->_wsaPacket[session->_wsaBufCount]->GetDataSize();
		session->_wsaBufCount++;
	}

	if (session->_wsaBufCount == 0)
	{
		InterlockedExchange(&session->_sendCnt, 0);
		return false;
	}

	int result = WSASend(session->_socket, session->_wsaSendBuf, session->_wsaBufCount, &sendBytes,
		flags, &session->_sendOverlapped, NULL);

	if (result == SOCKET_ERROR)
	{
		int errCode = WSAGetLastError();
		if (errCode != WSA_IO_PENDING)
		{
			if (errCode != 10054 && errCode != 10053)
			{
				Log(L"WSASend Error", errCode, dfLOG_LEVEL_ERROR);
			}
			DecrementRefCount(session);
			return false;
		}
	}

	return true;
}

BOOL CServer::SetSessionID(Session* session)
{
	LONGLONG sessionIndex = GetEmptyArrayIndex();
	if (sessionIndex == INVALID_INDEX)
		return false;

	LONGLONG sessionID = MakeID(sessionIndex);
	if (sessionID == INVALID_SESSION_ID)
		return false;

	_sessionArray[sessionIndex] = session;
	session->_ID = sessionID;
	return true;
}

BOOL CServer::AllocSession(Session** session, SOCKET socket, const SOCKADDR_IN& sockaddr)
{
	*session = _sessionPool->Alloc();
	if (*session == nullptr)
		return false;

	(*session)->_socket = socket;
	::memcpy(&(*session)->_sockaddr, &sockaddr, sizeof(SOCKADDR_IN));

	(*session)->_wsaBufCount = 0;
	(*session)->_sendCnt = 0;
	(*session)->_RefCount = 0;

	// SendQ 정리
	(*session)->_sendQ.Clear();

	if (SetSessionID(*session) == false)
	{
		DebugBreak();
	}
	InterlockedExchange8((CHAR*)&(*session)->_ReleaseFlag, false);
	return true;
}

VOID CServer::ReleaseSession(Session* session, const LONGLONG sessionID)
{
	// 1) 세션에 대한 CAS를 진행하며 Release가능 여부를 확인한다.
	if (InterlockedCompareExchange(reinterpret_cast<LONG*>(&session->_ReleaseFlag), (LONG)1, 0) == 1)
		return;

	// 2) 세션에 대한 RefCount를 다시한번 확인한다.
	if (InterlockedCompareExchange(&session->_RefCount, 0, 0) != 0)
	{
		InterlockedExchange(reinterpret_cast<LONG*>(&session->_ReleaseFlag), 0);
		return;
	}

	// Release Session
	session->Release();

	// Release Session Pool
	_sessionPool->Free(session);

	// Relesae Session Array
	LONGLONG sessionIndex; DecompositionID(sessionID, nullptr, &sessionIndex);
	if (session != _sessionArray[sessionIndex])
	{

	}

	_sessionArray[sessionIndex] = nullptr;
	InsertIndexToQueue(sessionIndex);

	// Call OnClientLeave
	OnClientLeave(sessionID);

	// Decrement SessionCount;
	InterlockedDecrement(&_sessionCount);
}

VOID CServer::LaunchWorkerThread()
{
	SetEvent(_launchEvent);
}

unsigned int CServer::AcceptThreadFunc()
{
	SOCKET          clientSocket;
	SOCKADDR_IN     clientaddr;
	int             addrlen = sizeof(clientaddr);
	DWORD           recvbytes, flags;

	DWORD result = WaitForSingleObject(_launchEvent, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		DebugBreak();
	}

	for (;;)
	{
		clientSocket = accept(_listenSocket, (SOCKADDR*)&clientaddr, &addrlen);
		if (clientSocket == INVALID_SOCKET) {
			DWORD errCode = WSAGetLastError();
			if (errCode != WSAENOTSOCK)
			{
				Log(L"Accept Error", errCode, dfLOG_LEVEL_ERROR);
			}
			break;
		}
		_acceptTotal += 1;

		BOOL result = OnConnectionRequest(clientaddr);
		if (result == false)
		{
			closesocket(clientSocket);
			continue;
		}

		if (WaitForSingleObject(_shutdownEvent, 0) == WAIT_OBJECT_0)
		{
			closesocket(clientSocket);
			break;
		}

		Session* newSession = nullptr;
		if (AllocSession(&newSession, clientSocket, clientaddr) == false)
		{
			Log(L"Allocate Error", 0, dfLOG_LEVEL_ERROR);
			DebugBreak();
			break;
		}

		CreateIoCompletionPort((HANDLE)clientSocket, _iocpHandle, (ULONG_PTR)newSession, 0);

		InterlockedIncrement(&_sessionCount);
		InterlockedIncrement(&_acceptTPSCount);

#ifdef DEBUG
		wcout << L"Accept Succcess , Session : " << newSession->_ID << endl;
#endif
		RecvPost(newSession);
		OnClientJoin(newSession->_ID);
	}

#ifdef DEBUG
	wcout << L"AceeptThread Terminate" << endl;
#endif

	return 0;
}

unsigned int CServer::WorkerThreadFunc()
{
	DWORD result = WaitForSingleObject(_launchEvent, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		DebugBreak();
	}

	for (;;)
	{
		DWORD		cbTransferred = 0;
		Session* session = nullptr;
		OVERLAPPED* overlapped = nullptr;

		DWORD retval = GetQueuedCompletionStatus(_iocpHandle, &cbTransferred,
			(PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);

		if (cbTransferred == 0 && session == 0 && overlapped == 0) break;
		if (session == nullptr) continue;
		session = AccessSession(session->_ID);
		if (session == nullptr)
			continue;

		if (retval == 0 || cbTransferred == 0)
		{
			if (retval == 0)
			{
				if (overlapped == nullptr)
				{
					DWORD errCode = WSAGetLastError();
					if (errCode != WSAECONNRESET && errCode != errCode != WSAECONNABORTED)
					{
						Log(L"GQCS Error, Error Code : ", errCode, dfLOG_LEVEL_ERROR);
					}
				}
			}
			// - GQCS에 실패하거나 (retval == 0), 송수신 처리량이 0인 경우엔(Transfferd == 0)
			//   Overlapped가 nullptr인지에 구분 없이 연결종료가 되어야 하므로 완료통지 Count만 줄인다.
		}
		else if (overlapped == &session->_recvOverlapped)
		{
			HandleRecv(session, cbTransferred);
		}
		else if (overlapped == &session->_sendOverlapped)
		{
			HandleSend(session, cbTransferred);
		}

		DecrementRefCount(session);
		DeaccessSession(session);
	}

#ifdef DEBUG
	wcout << L"WorkerThread Terminate" << endl;
#endif

	return 0;
}

unsigned int CServer::ShutDownThreadFunc()
{
	DWORD result = WaitForSingleObject(_shutdownEvent, INFINITE);
	if (result != WAIT_OBJECT_0)
	{
		Log(L"WaitForSingleObject Error in ShutdownThread, reuslt : ", result, dfLOG_LEVEL_ERROR);
	}

	OnShutDown();

	for (;;)
	{
		if (_sessionCount == 0)
		{
#ifdef DEBUG
			wcout << L"All Sessions are already Exit" << endl;
#endif
			for (int cnt = 0; cnt < _workerThreadCount - 2; cnt++)
			{
				PostQueuedCompletionStatus(_iocpHandle, 0, 0, 0);
			}
			break;
		}
	}

#ifdef DEBUG
	wcout << L"ShutDownThread Terminate" << endl;
#endif

	return 0;
}

// SessionArray에 session이 존재하면 반환, 없다면 nullptr 반환
Session* CServer::FindSession(const LONGLONG sessionID)
{
	LONGLONG sessionIndex;
	DecompositionID(sessionID, nullptr, &sessionIndex);
	return _sessionArray[sessionIndex];
}

Session* CServer::AccessSession(const LONGLONG sessionID)
{
	if (sessionID == -1)
	{
		return nullptr;
	}
	Session* session = FindSession(sessionID);
	if (session == nullptr)
	{
		session = FindSession(sessionID);
		if (session == nullptr)
		{
			return nullptr;
		}
	}
	// 1) 세션에 대한 참조 카운트를 증가한다.
	IncrementRefCount(session);

	// 2) ReleaseFlag가 활성화된 경우 접근을 중단한다.
	if (session->_ReleaseFlag == true)
	{
		// TODO : 중단한다.
		DecrementRefCount(session);
		return nullptr;
	}

	// 3) 이미 재사용되 다른 세션ID인 경우 접근을 중단한다.
	if (session->_ID != sessionID)
	{
		DecrementRefCount(session);
		return nullptr;
	}
	return session;
}

VOID    CServer::DeaccessSession(Session* session)
{
	DecrementRefCount(session);
}

VOID CServer::IncrementRefCount(Session* session)
{
	InterlockedIncrement(&session->_RefCount);
}

VOID CServer::DecrementRefCount(Session* session)
{
	LONG ret = InterlockedDecrement(&session->_RefCount);
	if (ret == 0)
	{
		ReleaseSession(session, session->_ID);
	}
}

LONGLONG CServer::GetEmptyArrayIndex()
{
	AcquireSRWLockExclusive(&_emptyIndexLock);

	if (_emptyIndexQueue.empty())
	{
		ReleaseSRWLockExclusive(&_emptyIndexLock);
		return INVALID_INDEX;
	}

	LONGLONG index = _emptyIndexQueue.front();
	_emptyIndexQueue.pop();

	ReleaseSRWLockExclusive(&_emptyIndexLock);

	return index;
}
VOID CServer::InsertIndexToQueue(const LONGLONG index)
{
	AcquireSRWLockExclusive(&_emptyIndexLock);

	_emptyIndexQueue.push(index);

	ReleaseSRWLockExclusive(&_emptyIndexLock);
}


// 세션 카운트와 인덱스를 ID로 조합
LONGLONG CServer::MakeID(const LONGLONG sessionIndex)
{
	LONGLONG newID = (_sessionNum << 20) | (sessionIndex);
	_sessionNum++;

	return newID;
}

// 세션 ID를 카운트와 인덱스로 분해
VOID CServer::DecompositionID(const LONGLONG sessionID, LONGLONG* sessionCount, LONGLONG* sessionIndex)
{
	if (sessionIndex != nullptr)
		*sessionIndex = (sessionID & 0x0000000000fffff);

	if (sessionCount != nullptr)
		*sessionCount = (sessionID >> 20);
}

static unsigned int __stdcall AcceptThread(void* arg)
{
	return reinterpret_cast<CServer*>(arg)->AcceptThreadFunc();
}

static unsigned int __stdcall WorkerThread(void* arg)
{
	return reinterpret_cast<CServer*>(arg)->WorkerThreadFunc();
}

static unsigned int __stdcall ShutDownThread(void* arg)
{
	return reinterpret_cast<CServer*>(arg)->ShutDownThreadFunc();
}
