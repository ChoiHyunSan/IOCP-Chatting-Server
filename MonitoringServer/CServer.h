#pragma once
#include "pch.h"
#include "Session.h"

/************************

       SERVER LIB

************************/

// #define DEBUG

class CServer
{
public:
    CServer(const WCHAR* fileName = L"ServerSetting.txt");
    ~CServer();


public:
    BOOL    Launch();
    VOID    ShutDown();    

    // 모니터링 항목 Getter 함수
    DWORD   GetSessionCount();
    DWORD   GetAcceptTPS();
    DWORD   GetRecvMessageTPS();
    DWORD   GetSendMessageTPS();

    // 컨텐츠 측에서 라이브러리로 요청하는 함수
    BOOL    Disconnect(const LONGLONG sessionID);
    BOOL    SendPacket(const LONGLONG sessionID, CPacket* packet);
    VOID    ReserveDisconnectSession(const LONGLONG sessionID, CPacket* packet);

    // 인터페이스 함수 
    virtual BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr) = 0;      // Accept 직후
    virtual VOID OnClientJoin(const LONGLONG sessionID) = 0;                // Accept 후 접속처리 완료 후 호출
    virtual VOID OnClientLeave(const LONGLONG sessionID) = 0;               // Release 후 호출
    virtual VOID OnMessage(const LONGLONG sessionID, CPacket* packet) = 0;  // 패킷 수신 완료 후
    virtual VOID OnError(const DWORD errCode, const WCHAR* str) = 0;        // 에러를 표시해줘야 하는 경우 호출
    virtual inline VOID OnSend(const LONGLONG sessionID, CPacket* packet) = 0;
    virtual VOID OnLaunch() = 0;
    virtual VOID OnShutDown() = 0;

protected:


private:
    // 라이브러리 리소스
    HANDLE                  _iocpHandle;            // IOCP 핸들
    SOCKET                  _listenSocket;          // Listen 소켓

    // 라이브러리 쓰레드
    HANDLE                 _acceptThread;           // Accept전용 쓰레드         
    HANDLE*                _workerThreads;          // IO 워커 쓰레드
    HANDLE                 _shutdownThread;         // Shutdown 쓰레드

    // 이벤트 객체
    HANDLE                  _shutdownEvent;
    HANDLE                  _launchEvent;

    // 서버 세팅 정보
    WCHAR                   _ServerFile[40];        // 파일 이름
    WCHAR                   _serverIP[20];          // IP 주소
    DWORD                   _serverPort;            // 포트 번호
    DWORD                   _bufferSize;            // 버퍼 크기
    DWORD                   _workerThreadCount;     // 워커 쓰레드 개수
    DWORD                   _activeThreadCount;     // IOCP 러닝쓰레드 설정 개수

    DWORD                   _sessionMaxCount;       // 세션 최대치
    DWORD                   _userMaxCount;          // 유저 최대치

    CHAR                    _packetCode;            // 패킷 코드
    BYTE                    _packetKey;             // 패킷 키
    en_SERVER_TYPE          _serverType;            // 서버 타입
    BOOL                    _nagleflag;             // 네이글 정보   

    // 세션 관리 객체
    Session**               _sessionArray;

    // queue<LONGLONG>          _emptyIndexQueue;
    CLockFreeQueue<LONGLONG> _emptyIndexQueue;
    // SRWLOCK                  _emptyIndexLock;

    LONGLONG                _sessionNum = 0;

    // 세션 오브젝트 풀
    MemoryPool<Session>*    _sessionPool;

    // 모니터링 변수
protected:
    DWORD       _recvTPSCount = 0;
    DWORD       _sendTPSCount = 0;
    DWORD       _acceptTPSCount = 0;

    LONG        _sessionCount = 0;
    INT64       _acceptTotal = 0;

private:
    // 라이브러리 초기화 함수
    BOOL        ServerSetting();
    BOOL        ParsingServerInfo();
    BOOL        SocketAndCPSetting();
    BOOL        ThreadSetting();

    // 라이브러리 정리 함수
    BOOL        ReleaseServer();
    
    // 송수신 처리 함수
    VOID        HandleRecv(Session* session, const DWORD recvBytes);
    VOID        HandleSend(Session* session, const DWORD sendBytes);

    VOID        SendMsgToContents(Session* session);
    VOID        HandleMsgByLanVersion(Session* session);
    VOID        HandleMsgByNetVersion(Session* session);

    VOID        AddHeaderByLanVersion(CPacket* packet);
    VOID        AddHeaderByNetVersion(CPacket* packet);

    VOID        RecvPost(Session* session);
    BOOL        SendPost(Session* session);

    // 라이브러리 내부처리 함수
    BOOL        SetSessionID(Session* session);
    BOOL        AllocSession(Session** session, SOCKET socket, const SOCKADDR_IN& sockaddr);
    VOID        ReleaseSession(Session* session, const LONGLONG sessionID);
    VOID        LaunchWorkerThread();

    Session*    FindSession(const LONGLONG sessionID);
    Session*    AccessSession(const LONGLONG sessionID);
    VOID        DeaccessSession(Session* session);

    VOID        IncrementRefCount(Session* session);
    VOID        DecrementRefCount(Session* session);

    LONGLONG    GetEmptyArrayIndex();
    VOID        InsertIndexToQueue(LONGLONG index);

    LONGLONG    MakeID(const LONGLONG sessionIndex);
    VOID        DecompositionID(const LONGLONG sessionID, LONGLONG* sessionCount, LONGLONG* sessionIndex);

private:
    // 쓰레드 함수
    unsigned int AcceptThreadFunc();
    unsigned int WorkerThreadFunc();
    unsigned int ShutDownThreadFunc();

    friend static unsigned int WINAPI AcceptThread(void* arg);
    friend static unsigned int WINAPI WorkerThread(void* arg);
    friend static unsigned int WINAPI ShutDownThread(void* arg);
};

// 쓰레드 함수
static unsigned int WINAPI AcceptThread(void* arg);
static unsigned int WINAPI WorkerThread(void* arg);
static unsigned int WINAPI ShutDownThread(void* arg);