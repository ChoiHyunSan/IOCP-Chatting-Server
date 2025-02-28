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

    // ����͸� �׸� Getter �Լ�
    DWORD   GetSessionCount();
    DWORD   GetAcceptTPS();
    DWORD   GetRecvMessageTPS();
    DWORD   GetSendMessageTPS();

    // ������ ������ ���̺귯���� ��û�ϴ� �Լ�
    BOOL    Disconnect(const LONGLONG sessionID);
    BOOL    SendPacket(const LONGLONG sessionID, CPacket* packet);
    VOID    ReserveDisconnectSession(const LONGLONG sessionID, CPacket* packet);

    // �������̽� �Լ� 
    virtual BOOL OnConnectionRequest(const SOCKADDR_IN& sockAddr) = 0;      // Accept ����
    virtual VOID OnClientJoin(const LONGLONG sessionID) = 0;                // Accept �� ����ó�� �Ϸ� �� ȣ��
    virtual VOID OnClientLeave(const LONGLONG sessionID) = 0;               // Release �� ȣ��
    virtual VOID OnMessage(const LONGLONG sessionID, CPacket* packet) = 0;  // ��Ŷ ���� �Ϸ� ��
    virtual VOID OnError(const DWORD errCode, const WCHAR* str) = 0;        // ������ ǥ������� �ϴ� ��� ȣ��
    virtual inline VOID OnSend(const LONGLONG sessionID, CPacket* packet) = 0;
    virtual VOID OnLaunch() = 0;
    virtual VOID OnShutDown() = 0;

protected:


private:
    // ���̺귯�� ���ҽ�
    HANDLE                  _iocpHandle;            // IOCP �ڵ�
    SOCKET                  _listenSocket;          // Listen ����

    // ���̺귯�� ������
    HANDLE                 _acceptThread;           // Accept���� ������         
    HANDLE*                _workerThreads;          // IO ��Ŀ ������
    HANDLE                 _shutdownThread;         // Shutdown ������

    // �̺�Ʈ ��ü
    HANDLE                  _shutdownEvent;
    HANDLE                  _launchEvent;

    // ���� ���� ����
    WCHAR                   _ServerFile[40];        // ���� �̸�
    WCHAR                   _serverIP[20];          // IP �ּ�
    DWORD                   _serverPort;            // ��Ʈ ��ȣ
    DWORD                   _bufferSize;            // ���� ũ��
    DWORD                   _workerThreadCount;     // ��Ŀ ������ ����
    DWORD                   _activeThreadCount;     // IOCP ���׾����� ���� ����

    DWORD                   _sessionMaxCount;       // ���� �ִ�ġ
    DWORD                   _userMaxCount;          // ���� �ִ�ġ

    CHAR                    _packetCode;            // ��Ŷ �ڵ�
    BYTE                    _packetKey;             // ��Ŷ Ű
    en_SERVER_TYPE          _serverType;            // ���� Ÿ��
    BOOL                    _nagleflag;             // ���̱� ����   

    // ���� ���� ��ü
    Session**               _sessionArray;

    // queue<LONGLONG>          _emptyIndexQueue;
    CLockFreeQueue<LONGLONG> _emptyIndexQueue;
    // SRWLOCK                  _emptyIndexLock;

    LONGLONG                _sessionNum = 0;

    // ���� ������Ʈ Ǯ
    MemoryPool<Session>*    _sessionPool;

    // ����͸� ����
protected:
    DWORD       _recvTPSCount = 0;
    DWORD       _sendTPSCount = 0;
    DWORD       _acceptTPSCount = 0;

    LONG        _sessionCount = 0;
    INT64       _acceptTotal = 0;

private:
    // ���̺귯�� �ʱ�ȭ �Լ�
    BOOL        ServerSetting();
    BOOL        ParsingServerInfo();
    BOOL        SocketAndCPSetting();
    BOOL        ThreadSetting();

    // ���̺귯�� ���� �Լ�
    BOOL        ReleaseServer();
    
    // �ۼ��� ó�� �Լ�
    VOID        HandleRecv(Session* session, const DWORD recvBytes);
    VOID        HandleSend(Session* session, const DWORD sendBytes);

    VOID        SendMsgToContents(Session* session);
    VOID        HandleMsgByLanVersion(Session* session);
    VOID        HandleMsgByNetVersion(Session* session);

    VOID        AddHeaderByLanVersion(CPacket* packet);
    VOID        AddHeaderByNetVersion(CPacket* packet);

    VOID        RecvPost(Session* session);
    BOOL        SendPost(Session* session);

    // ���̺귯�� ����ó�� �Լ�
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
    // ������ �Լ�
    unsigned int AcceptThreadFunc();
    unsigned int WorkerThreadFunc();
    unsigned int ShutDownThreadFunc();

    friend static unsigned int WINAPI AcceptThread(void* arg);
    friend static unsigned int WINAPI WorkerThread(void* arg);
    friend static unsigned int WINAPI ShutDownThread(void* arg);
};

// ������ �Լ�
static unsigned int WINAPI AcceptThread(void* arg);
static unsigned int WINAPI WorkerThread(void* arg);
static unsigned int WINAPI ShutDownThread(void* arg);