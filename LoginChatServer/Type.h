#pragma once
#include "CommonProtocol.h"
#include <Windows.h>

/************************
    ChatServer Define
*************************/
#define dfHEARTBEAT_TIME 40000
#define dfHEARTBEAT_LOOPTIME 100

/************************
         구조체
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
        _sessionID = sessionID;
        _accountNo = -1;
        _sX = 50;
        _sY = 50;

        memset(_ID, 0, sizeof(_ID));
        memset(_NickName, 0, sizeof(_NickName));

        _prevHeartBeat = GetTickCount64();
        _activeFlag = true;
    }

    INT64     _sessionID;
    INT64     _accountNo;
    WCHAR     _ID[20];
    WCHAR     _NickName[20];
    char      _sessionKey[64];

    WORD      _sX;
    WORD      _sY;
    ULONGLONG _prevHeartBeat;

    BOOL      _activeFlag;
    CRITICAL_SECTION  _lock;
    std::future<cpp_redis::reply> _redisFuture;
};

/************************
         Enum
************************/
enum class JOB_TYPE : BYTE
{
    C = 0,
    S
};

/************************
      서버 <-> 클라
*************************/
enum class CLIENT_JOB : INT64
{
    LOGIN = 0,
    MOVE,
    Message,
    HEARTBEAT
};

#define dfJOB_LOGIN     en_PACKET_CS_CHAT_REQ_LOGIN
#define dfJOB_MOVE      en_PACKET_CS_CHAT_REQ_SECTOR_MOVE
#define dfJOB_Message   en_PACKET_CS_CHAT_REQ_MESSAGE
#define dfJOB_HEARTBEAT en_PACKET_CS_CHAT_REQ_HEARTBEAT

/************************
      서버 <-> 서버
*************************/

enum class SERVER_JOB : INT64
{
    ACCEPT = 0,
    RELEASE, 
    HEARTBEAT
};
