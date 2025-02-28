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
    vector<thread>                      _threads;
    unordered_map<INT64, ClientInfo*>   _clientMap;
    SRWLOCK                             _mapLock;

private:
    void SendUnicast(const INT64 sessionID, CPacket* packet);
    void SendToMonitor(CPacket* packet);

    /************************
             컨텐츠
    ************************/
    void HandlePacket_ClientLogin(ClientInfo* info, CPacket* packet);
    void CreatePacket_ClientLogin(CPacket* packet, BYTE status);
    void CreatePacket_DataUpdate(CPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp);
};

