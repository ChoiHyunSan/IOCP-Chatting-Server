#pragma once
#include "CServer.h"
#include "Type.h"

class CMonitorServer;

class CLanMonitorServer : public CServer
{
public:
    CLanMonitorServer(CMonitorServer* NetServer, const WCHAR* fileName = L"ServerSetting.txt");
    ~CLanMonitorServer();

public:
    // 인터페이스 함수 
    virtual BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept 직후
    virtual VOID OnClientJoin(const LONGLONG sessionID);                // Accept 후 접속처리 완료 후 호출
    virtual VOID OnClientLeave(const LONGLONG sessionID);               // Release 후 호출
    virtual VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // 패킷 수신 완료 후
    virtual VOID OnError(const DWORD errCode, const WCHAR* str);        // 에러를 표시해줘야 하는 경우 호출
    virtual inline VOID OnSend(const LONGLONG sessionID, CPacket* packet);
    virtual VOID OnLaunch();
    virtual VOID OnShutDown();

private:
    CMonitorServer*                     _netServer;

    vector<thread>                      _threads;
    unordered_map<INT64, ServerInfo*>   _serverMap;
    SRWLOCK                             _mapLock;
    unordered_map<INT64, MonitorInfo*>  _monitorInfoMap;
    CMonitor                            _monitor;

    /************************
                DB
    ************************/
    SQL* _logDB;
    char _serverIP[20];
    char _user[40];
    char _password[40];
    char _table[40];
    int	 _port;

private:
    /************************
            컨텐츠
    ************************/
    void HandlePacket_ServerLogin(ServerInfo* info, CPacket* packet);
    void HandlePacket_DataUpdate(ServerInfo* info, CPacket* packet);

    void CreatePacket_DataUpdate(CPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);

    friend void DBThreadFunc(CLanMonitorServer* server);
    friend void MonitorThreadFunc(CLanMonitorServer* server);
    
    void DBFunc();
    void MontiorFunc();
};
    
void DBThreadFunc(CLanMonitorServer* server);
void MonitorThreadFunc(CLanMonitorServer* server);


