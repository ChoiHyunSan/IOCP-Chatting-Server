#pragma once
#include <Windows.h>

/*******************************
      ���� �ʱⰪ ���� ����
********************************/
#define SERVER_PORT             L"SERVER_PORT"
#define SERVER_IP               L"SERVER_IP"

#define BUFFER_SIZE             L"BUFFER_SIZE"

#define WORKER_THREAD_COUNT     L"IOCP_WORKER_THREAD"
#define ACTIVE_THREAD_COUNT     L"IOCP_ACTICVE_THREAD"

#define SESSION_MAX             L"SESSION_MAX"
#define USER_MAX                L"USER_MAX"

#define PACKET_CODE             L"PACKET_CODE"
#define PACKET_KEY              L"PACKET_KEY"

#define NAGLE                   L"NAGLE"
#define SERVER_TYPE             L"SERVER_TYPE"

/*******************************
      ���� ��Ŷ ����ü
********************************/

#pragma pack(push, 1)
struct PacketHeader    // LAM ����
{
    SHORT   _len;
};

struct NetPacketHeader
{
    CHAR   _code;
    SHORT  _len;
    BYTE   _randKey;
    BYTE   _checksum;
};

#pragma pack (pop)
/*******************************
         ��Ÿ ���� ��
********************************/
#define MAX_ID		        10000
#define OBJECT_POOL_MIN     500

#define INVALID_SESSION_ID  -1
#define INVALID_INDEX       -1
#define out

enum class en_SERVER_TYPE
{
    LAN = '0',
    NET = '1'
};