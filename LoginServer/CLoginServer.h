#pragma once
#include "CServer.h"
#include "Type.h"

#define MONITOR
// #define DISCONNECT_BY_SERVER
class CLoginServer : public CServer
{
public:
    CLoginServer();
    ~CLoginServer();

private:
    BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept 직후
    VOID OnClientJoin(const LONGLONG sessionID);                // Accept 후 접속처리 완료 후 호출
    VOID OnClientLeave(const LONGLONG sessionID);               // Release 후 호출
    VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // 패킷 수신 완료 후
    VOID OnError(const DWORD errCode, const WCHAR* str);        // 에러를 표시해줘야 하는 경우 호출
    inline VOID OnSend(const LONGLONG sessionID, CPacket* packet);

    VOID OnLaunch();
    VOID OnShutDown();

    /************************
              서버
    ************************/
    void    MonitorFunc();
    void    HeartbeatFunc();
    bool    Initialize();

    void    SendUnicast(CPacket* packet, const INT64 sessionID);
    void    DBFunc();
    void    RedisFunc();

    /************************
             컨텐츠
    *************************/
    inline void     HandleClientJob(const LONGLONG sessionID, CPacket* packet);
    void            AcceptUser(const LONGLONG sessionID);
    void            ReleaseUser(const LONGLONG sessionID);


private:
    /************************
             컨텐츠
    *************************/
    vector<std::thread>             _threads;
    HANDLE                          _shutdownEvent;

    MemoryPool<User>*               _userPool;
    unordered_map<INT64, User*>     _userMap;
    CRITICAL_SECTION                _userMapLock;

    cpp_redis::client               _redisClient;

    HANDLE                          _dbEvent;
    CLockFreeQueue<INT64>           _dbQueue;

    HANDLE                          _redisEvent;
    CLockFreeQueue<INT64>           _redisQueue;

    /************************
                DB
    ************************/
    SQL* _playerDB;
    char _serverIP[20];
    char _user[40];
    char _password[40];
    char _table[40];
    int	 _port;

    /************************
            모니터링
    *************************/
    CMonitor _monitor;
    LONG     _updateTPS = 0;
    LONG     _userCount = 0;
    LONG     _dbTPS = 0;
    LONG     _redisTPS = 0;
    LONG     _heartbeatCnt = 0;

    friend class PacketHandler;
    friend void MonitorThreadFunc(CLoginServer* arg);
    friend void HeartBeatThreadFunc(CLoginServer* arg);
    friend void DBThreadFunc(CLoginServer* arg);
    friend void RedisThreadFunc(CLoginServer* arg);
};

void MonitorThreadFunc(CLoginServer* arg);
void HeartBeatThreadFunc(CLoginServer* arg);
void DBThreadFunc(CLoginServer* arg);
void RedisThreadFunc(CLoginServer* arg);

extern CLoginServer g_server;