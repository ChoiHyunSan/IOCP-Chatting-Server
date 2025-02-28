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
    // �������̽� �Լ� 
    virtual BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr);      // Accept ����
    virtual VOID OnClientJoin(const LONGLONG sessionID);                // Accept �� ����ó�� �Ϸ� �� ȣ��
    virtual VOID OnClientLeave(const LONGLONG sessionID);               // Release �� ȣ��
    virtual VOID OnMessage(const LONGLONG sessionID, CPacket* packet);  // ��Ŷ ���� �Ϸ� ��
    virtual VOID OnError(const DWORD errCode, const WCHAR* str);        // ������ ǥ������� �ϴ� ��� ȣ��
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
            ������
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


