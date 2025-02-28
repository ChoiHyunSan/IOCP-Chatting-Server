#pragma once
#include "pch.h"

struct ClientInfo
{
    INT64   _sessionID;     // ���� ID
};

struct ServerInfo
{
    ServerInfo(){ _lastRecvTime = GetTickCount64(); }

    INT64   _sessionID;     // ���� ID
    BOOL    _serverFlag;    // ���� : 1 , ����͸� Ŭ�� : 0
    int     _serverNo;      // ������ ��� ������ȣ�� �����Ѵ�.
    ULONGLONG   _lastRecvTime;
};

struct MonitorInfo
{
    MonitorInfo()
    {
        InitializeSRWLock(&_lock);
    }

    void Init()
    {
        _cnt = 0;
        _max = 0;
        _min = MAXINT32;
        memset(_vArr, 0, sizeof(_vArr));
    }
    void update(int value)
    {
        if(_cnt < 100)
            _vArr[_cnt] = value;
        _cnt += 1;
        _max = max(_max, value);
        _min = min(_min, value);
    }
    void GetMonitorValue(int* avg, int* max, int* min)
    {
        int sum = 0;
        for (int i = 0; i < _cnt; i++)
            sum += _vArr[_cnt];

        if(_cnt > 0)
            *avg = (sum / _cnt);
        
        *max = _max;
        *min = _min;
    }

    int     _cnt = 0;
    int     _vArr[100];
    int     _max = 0;
    int     _min = MAXINT32;
    int     _serverNo = -1;
    SRWLOCK _lock;

};