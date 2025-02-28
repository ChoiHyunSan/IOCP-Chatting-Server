#pragma once
#include "CServer.h"
#include "CommonProtocol.h"
#include "Type.h"


class CMonitorServer : public CServer
{
    friend class CLanMonitorServer;
public:
    CMonitorServer();
    ~CMonitorServer();

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
    vector<thread>                      _threads;
    unordered_map<INT64, ClientInfo*>   _clientMap;
    SRWLOCK                             _mapLock;

private:
    void SendUnicast(const INT64 sessionID, CPacket* packet);
    void SendToMonitor(CPacket* packet);

    /************************
             ������
    ************************/
    void HandlePacket_ClientLogin(ClientInfo* info, CPacket* packet);
    void CreatePacket_ClientLogin(CPacket* packet, BYTE status);
    void CreatePacket_DataUpdate(CPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);
};

