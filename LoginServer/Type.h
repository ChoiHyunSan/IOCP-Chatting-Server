#pragma once
#include "CommonProtocol.h"
#include <Windows.h>

/************************
    ChatServer Define
*************************/
#define dfHEARTBEAT_TIME 40000
#define dfHEARTBEAT_LOOPTIME 100

/************************
         ����ü
************************/
struct User
{
    User()
    {
        InitializeCriticalSection(&_lock);
    }
    ~User()
    {
        DeleteCriticalSection(&_lock);
    }

    void Init(INT64 sessionID)
    {
        memset(_ID, 0, sizeof(_ID));
        memset(_NickName, 0, sizeof(_NickName));

        _accountNo = -1;
        _sessionID = sessionID;

        _prevHeartBeat = GetTickCount64();
    }

    INT64     _sessionID;
    INT64     _accountNo;

    // ����Ű �� �ѱ���� Ȯ�ο�
    WCHAR     _ID[20];
    WCHAR     _NickName[20];
    char      _sessionKey[64];
    ULONGLONG _prevHeartBeat;

    CRITICAL_SECTION  _lock;
};

/************************
      ���� <-> Ŭ��
*************************/
#define dfJOB_LOGIN     en_PACKET_CS_LOGIN_REQ_LOGIN


/************************
      ���� <-> ����
*************************/

