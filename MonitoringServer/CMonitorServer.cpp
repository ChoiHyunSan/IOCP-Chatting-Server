#include "CMonitorServer.h"

CMonitorServer::CMonitorServer()
{
	InitializeSRWLock(&_mapLock);
}

CMonitorServer::~CMonitorServer()
{
	for (auto& thread : _threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

BOOL CMonitorServer::OnConnectionRequest(const SOCKADDR_IN& sockAddr)
{
	return true;
}

VOID CMonitorServer::OnClientJoin(const LONGLONG sessionID)
{

}

VOID CMonitorServer::OnClientLeave(const LONGLONG sessionID)
{
	AcquireSRWLockExclusive(&_mapLock);
	if (_clientMap.find(sessionID) != _clientMap.end())
	{
		ClientInfo* info = _clientMap[sessionID];
		_clientMap.erase(sessionID);
		delete info;
	}
	ReleaseSRWLockExclusive(&_mapLock);

	wcout << "Disconnect One Monitoring Client, SessionID : " << sessionID << endl;
}

VOID CMonitorServer::OnMessage(const LONGLONG sessionID, CPacket* packet)
{
	AcquireSRWLockExclusive(&_mapLock);
	if (_clientMap.find(sessionID) == _clientMap.end())
	{
		_clientMap[sessionID] = new ClientInfo;
		_clientMap[sessionID]->_sessionID = sessionID;
	}
	ClientInfo* info = _clientMap[sessionID];
	ReleaseSRWLockExclusive(&_mapLock);

	WORD type;
	(*packet) >> type;

	switch (type)
	{
	case en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN:
		HandlePacket_ClientLogin(info, packet);
		break;
	default:
		Log(L"Wrong CS MSG Packet Type", type);
		break;
	}
}

VOID CMonitorServer::OnError(const DWORD errCode, const WCHAR* str)
{

}

inline VOID CMonitorServer::OnSend(const LONGLONG sessionID, CPacket* packet)
{

}

VOID CMonitorServer::OnLaunch()
{

}

VOID CMonitorServer::OnShutDown()
{

}

void CMonitorServer::SendUnicast(const INT64 sessionID, CPacket* packet)
{
	packet->AddRefCount();
	if (SendPacket(sessionID, packet) == false)
	{
		CPacket::Free(packet);
	}
}

void CMonitorServer::SendToMonitor(CPacket* packet)
{
	AcquireSRWLockExclusive(&_mapLock);
	for (unordered_map<INT64, ClientInfo*>::iterator iter = _clientMap.begin();
		iter != _clientMap.end(); iter++)
	{
		SendUnicast((*iter).second->_sessionID, packet);
	}
	ReleaseSRWLockExclusive(&_mapLock);
}

void CMonitorServer::HandlePacket_ClientLogin(ClientInfo* info, CPacket* packet)
{
	char key[32];
	(*packet).GetData(key, 32);

	// TODO : 세션키 확인하기
	bool auth = true;
	if (memcmp("ajfw@!cv980dSZ[fje#@fdj123948djf", key, 32) != 0)
		auth = false;

	// 인증패킷 보내기
	CPacket* loginPacket = CPacket::Alloc();
	if (auth)
	{
		CreatePacket_ClientLogin(loginPacket, dfMONITOR_TOOL_LOGIN_OK);
	}
	else
	{
		CreatePacket_ClientLogin(loginPacket, dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY);
	}
	SendUnicast(info->_sessionID, loginPacket);
	CPacket::Free(loginPacket);

	wcout << "Connect One Monitoring Client, SessionID : " << info->_sessionID << endl;
}

void CMonitorServer::CreatePacket_ClientLogin(CPacket* packet, BYTE status)
{
	(*packet) << (WORD)en_PACKET_CS_MONITOR_TOOL_RES_LOGIN;
	(*packet) << status;
}

void CMonitorServer::CreatePacket_DataUpdate(CPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	(*packet) << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	(*packet) << serverNo;
	(*packet) << dataType;
	(*packet) << dataValue;
	(*packet) << timeStamp;
}
