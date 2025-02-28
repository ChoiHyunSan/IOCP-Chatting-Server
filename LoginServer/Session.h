#pragma once
#include "pch.h"

class Session
{
public:
    OVERLAPPED   _recvOverlapped;
    OVERLAPPED   _sendOverlapped;

    SOCKET       _socket;

    CRingBuffer  _recvRingBuffer;
    CLockFreeQueue<CPacket*> _sendQ;

    CPacket*     _wsaPacket[500];       // send에 사용한 패킷 정보 저장
    DWORD        _wsaBufCount = 0;      // send에 사용한 패킷 개수 저장

    WSABUF       _wsaSendBuf[500];
    WSABUF       _wsaRecvBuf[2];

    SOCKADDR_IN  _sockaddr;

    LONGLONG     _ID;
    DWORD        _sendCnt = 0;
    LONG         _RefCount = 0;


    BOOL        _ReleaseFlag = false;

    BOOL        _reserveDisconnect = false;
    CPacket*    _lastPacket = nullptr;
    Session()
    {
    }
    ~Session()
    {
    }

    VOID Release()
    {
        _ID = -1;
        closesocket(_socket);
        _socket = INVALID_SOCKET;
        _reserveDisconnect = false;
        _lastPacket = nullptr;
        _recvRingBuffer.ClearBuffer();

        CPacket* packet;
        while (true)
        {
            if (_sendQ.Dequeue(packet) == false)
                break;
            CPacket::Free(packet);
        }

        if (_wsaBufCount != 0)
        {
            for (int i = 0; i < _wsaBufCount; i++)
            {
                CPacket* packet = _wsaPacket[i];
                if (packet == nullptr)
                    break;

                CPacket::Free(_wsaPacket[i]);
                _wsaPacket[i] = nullptr;
            }
        }
    }
};

