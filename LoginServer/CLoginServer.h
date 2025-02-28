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
    BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept ����
    VOID OnClientJoin(const LONGLONG sessionID);                // Accept �� ����ó�� �Ϸ� �� ȣ��
    VOID OnClientLeave(const LONGLONG sessionID);               // Release �� ȣ��
    VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // ��Ŷ ���� �Ϸ� ��
    VOID OnError(const DWORD errCode, const WCHAR* str);        // ������ ǥ������� �ϴ� ��� ȣ��
    inline VOID OnSend(const LONGLONG sessionID, CPacket* packet);

    VOID OnLaunch();
    VOID OnShutDown();

    /************************
              ����
    ************************/
    void    MonitorFunc();
    void    HeartbeatFunc();
    bool    Initialize();

    void    SendUnicast(CPacket* packet, const INT64 sessionID);
    void    DBFunc();
    void    RedisFunc();

    /************************
             ������
    *************************/
    inline void     HandleClientJob(const LONGLONG sessionID, CPacket* packet);
    void            AcceptUser(const LONGLONG sessionID);
    void            ReleaseUser(const LONGLONG sessionID);


private:
    /************************
             ������
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
            ����͸�
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