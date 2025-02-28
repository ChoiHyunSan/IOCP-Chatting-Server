#pragma once
#include "CServer.h"
#include "CommonProtocol.h"
#include "Type.h"
#include "PacketHandler.h"

/******************************

       Login Chat Server

******************************/
// - ä�ü��� ����
//   ��Ƽ ������ ������ �����Ѵ�.

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
             ������
    ************************/
    MemoryPool<User>*               _userPool;
    unordered_map<INT64, User*>     _userMap;
    CRITICAL_SECTION                _userMapLock;

    list<User*>                     _sectorList[dfSECTOR_SIZE][dfSECTOR_SIZE];
    CRITICAL_SECTION                _sectorLock[dfSECTOR_SIZE][dfSECTOR_SIZE];
    INT32                           _dx[3] = { -1,0,1 };
    INT32                           _dy[3] = { -1,0,1 };

    /************************
              ����
    ************************/
    HANDLE              _hHeartBeatThread;
    HANDLE              _hMonitorThread;

    HANDLE              _launchEvent;
    HANDLE              _shutDownEvent;

    cpp_redis::client   _redisClient;
    HANDLE              _redisCP;           // Redis �񵿱� ������ ���� Completion Port
    vector<std::thread> _redisThreads;      // Redis �񵿱� ó���� ����ϴ� ������
    CLockFreeQueue<LONGLONG> _redisQueue;

    /************************
            ����͸�
    ************************/
    CMonitor _monitor;
    LONG    _updateTPS = 0;
    LONG    _userCount = 0;
    LONG    _heartbeatDisconnect = 0;


private:
    /************************
           �ݹ� �Լ�
    ************************/
    BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept ����
    VOID OnClientJoin(const LONGLONG sessionID);                // Accept �� ����ó�� �Ϸ� �� ȣ��
    VOID OnClientLeave(const LONGLONG sessionID);               // Release �� ȣ��
    VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // ��Ŷ ���� �Ϸ� ��
    VOID OnError(const DWORD errCode, const WCHAR* str);        // ������ ǥ������� �ϴ� ��� ȣ��

    VOID OnLaunch();
    VOID OnShutDown();

    /************************
              ����
    ************************/
    BOOL                Initialize();
    UINT32              HeartBeatThreadFunc();
    UINT32              MonitorThreadFunc();
    UINT32              RedisThreadFunc();
    void                SendUnicast(CPacket* packet, const INT64 sessionID);
    void                SendSector(CPacket* packet, const int sX, const int sY);
    void                SendAroundSector(CPacket* packet, User* user);


    /************************
             ������
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