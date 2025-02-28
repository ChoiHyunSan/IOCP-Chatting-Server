#pragma once
#include "CServer.h"
#include "CommonProtocol.h"
#include "Type.h"
#include "PacketHandler.h"

/******************************

       Login Chat Server

******************************/
// - 채팅서버 구현
//   멀티 스레드 구조로 구현한다.

#define dfSECTOR_SIZE 50

#define MONITOR
class MultiThreadChatServer : public CServer
{
    friend class PacketHandler;

public:
    MultiThreadChatServer();
    ~MultiThreadChatServer();

private:
    /************************
             컨텐츠
    ************************/
    MemoryPool<User>*               _userPool;
    unordered_map<INT64, User*>     _userMap;
    CRITICAL_SECTION                _userMapLock;

    list<User*>                     _sectorList[dfSECTOR_SIZE][dfSECTOR_SIZE];
    CRITICAL_SECTION                _sectorLock[dfSECTOR_SIZE][dfSECTOR_SIZE];
    INT32                           _dx[3] = { -1,0,1 };
    INT32                           _dy[3] = { -1,0,1 };

    /************************
              서버
    ************************/
    HANDLE              _hHeartBeatThread;
    HANDLE              _hMonitorThread;

    HANDLE              _launchEvent;
    HANDLE              _shutDownEvent;

    cpp_redis::client   _redisClient;
    HANDLE              _redisCP;           // Redis 비동기 수행을 위한 Completion Port
    vector<std::thread> _redisThreads;      // Redis 비동기 처리를 대기하는 쓰레드
    CLockFreeQueue<LONGLONG> _redisQueue;

    /************************
            모니터링
    ************************/
    CMonitor _monitor;
    LONG    _updateTPS = 0;
    LONG    _userCount = 0;
    LONG    _heartbeatDisconnect = 0;


private:
    /************************
           콜백 함수
    ************************/
    BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept 직후
    VOID OnClientJoin(const LONGLONG sessionID);                // Accept 후 접속처리 완료 후 호출
    VOID OnClientLeave(const LONGLONG sessionID);               // Release 후 호출
    VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // 패킷 수신 완료 후
    VOID OnError(const DWORD errCode, const WCHAR* str);        // 에러를 표시해줘야 하는 경우 호출

    VOID OnLaunch();
    VOID OnShutDown();

    /************************
              서버
    ************************/
    BOOL                Initialize();
    UINT32              HeartBeatThreadFunc();
    UINT32              MonitorThreadFunc();
    UINT32              RedisThreadFunc();
    void                SendUnicast(CPacket* packet, const INT64 sessionID);
    void                SendSector(CPacket* packet, const int sX, const int sY);
    void                SendAroundSector(CPacket* packet, User* user);


    /************************
             컨텐츠
    *************************/
    inline void     HandleClientJob(const LONGLONG sessionID, CPacket* packet);
    void            AcceptUser(const LONGLONG sessionID);
    void            ReleaseUser(const LONGLONG sessionID);
    void            MoveUser(const WORD destX, const WORD destY, User* user);
    void            SetUserInSector(const WORD sX, const WORD sY, User* user);
    void            DeleteUserInCurrentSector(User* user);
    void            HeartbeatCheck();
    void            HandleRequestID(User* user);
    void            RequestCheckID(LONGLONG sessionID);

    friend static UINT32 WINAPI HeartBeatThread(VOID* arg);
    friend static UINT32 WINAPI MonitorThread(void* arg);
    friend static UINT32 WINAPI RedisThread(void* arg); 
};

static UINT32 WINAPI HeartBeatThread(VOID* arg);
static UINT32 WINAPI MonitorThread(void* arg);
static UINT32 WINAPI RedisThread(void* arg);

extern MultiThreadChatServer g_server;