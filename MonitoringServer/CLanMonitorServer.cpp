#include "CLanMonitorServer.h"
#include "CMonitorServer.h"
#include "CommonProtocol.h"

CLanMonitorServer::CLanMonitorServer(CMonitorServer* NetServer, const WCHAR* fileName)
	: CServer(fileName), _netServer(NetServer)
{
	InitializeSRWLock(&_mapLock);

	// DB ����
	{
		CParsor parsor;
		BOOL result = parsor.LoadFile("DBSetting.txt");
		if (result == false)
		{
			Log(L"Load DBSetting file failed", 0, dfLOG_LEVEL_ERROR);
			DebugBreak();
		}
		if (!parsor.GetValue("SERVER_IP", (int*)_serverIP, sizeof(_serverIP))
			|| !parsor.GetValue("USER", (int*)_user, sizeof(_user))
			|| !parsor.GetValue("PASSWORD", (int*)_password, sizeof(_password))
			|| !parsor.GetValue("TABLE", (int*)_table, sizeof(_table))
			|| !parsor.GetValue("PORT", &_port))
		{
			Log(L"Set DB value failed", 0, dfLOG_LEVEL_ERROR);
			DebugBreak();
		}
		_logDB = new SQL(_serverIP, _user, _password, _table, _port);
	}

	// ����͸� ��� ����
	{
		// �α��� ����
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_LOGIN_SESSION, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS, new MonitorInfo()));

		// ���� ����
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_SERVER_RUN, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_SERVER_CPU, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_SERVER_MEM, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_SESSION, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_GAME_PACKET_POOL, new MonitorInfo()));

		// ä�� ����
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_SESSION, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_PLAYER, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL, new MonitorInfo()));
		
		// ���� ��ǻ��
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND, new MonitorInfo()));
		_monitorInfoMap.insert(make_pair<INT64, MonitorInfo*>(dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY, new MonitorInfo()));
	}

	// ������ ����
	_threads.push_back(thread(DBThreadFunc, this));
	_threads.push_back(thread(MonitorThreadFunc, this));
}

CLanMonitorServer::~CLanMonitorServer()
{
	for (auto& thread : _threads)
	{
		if (thread.joinable())
			thread.join();
	}
}

BOOL CLanMonitorServer::OnConnectionRequest(const SOCKADDR_IN& sockAddr)
{
	return true;
}

VOID CLanMonitorServer::OnClientJoin(const LONGLONG sessionID)
{

}

VOID CLanMonitorServer::OnClientLeave(const LONGLONG sessionID)
{
	AcquireSRWLockExclusive(&_mapLock);
	if (_serverMap.find(sessionID) != _serverMap.end())
	{
		ServerInfo* info = _serverMap[sessionID];

		wcout << "Server Disconnect, ServerNo : " << info->_serverNo << endl;
		_serverMap.erase(sessionID);
		delete info;
	}
	ReleaseSRWLockExclusive(&_mapLock);
}

VOID CLanMonitorServer::OnMessage(const LONGLONG sessionID, CPacket* packet)
{
	AcquireSRWLockExclusive(&_mapLock);
	if (_serverMap.find(sessionID) == _serverMap.end())
	{
		_serverMap[sessionID] = new ServerInfo;
		_serverMap[sessionID]->_sessionID = sessionID;
	}
	ServerInfo* info = _serverMap[sessionID];
	ReleaseSRWLockExclusive(&_mapLock);

	info->_lastRecvTime = GetTickCount64();

	WORD type;
	(*packet) >> type;

	switch (type)
	{
	case en_PACKET_SS_MONITOR_LOGIN:
		HandlePacket_ServerLogin(info, packet);
		break;
	case en_PACKET_SS_MONITOR_DATA_UPDATE:
		HandlePacket_DataUpdate(info, packet);
		break;
	default:
		wcout << "Wrong Type Packet " << type << endl;
		break;
	}
}

VOID CLanMonitorServer::OnError(const DWORD errCode, const WCHAR* str)
{

}

inline VOID CLanMonitorServer::OnSend(const LONGLONG sessionID, CPacket* packet)
{

}

VOID CLanMonitorServer::OnLaunch()
{
	for (;;)
	{
		Sleep(1000);
		
		vector<INT64> v;

		AcquireSRWLockExclusive(&_mapLock);
		for (unordered_map<INT64, ServerInfo*>::iterator iter = _serverMap.begin();
			iter != _serverMap.end(); iter++)
		{
			if (GetTickCount64() - (*iter).second->_lastRecvTime > 10000)
			{
				v.push_back((*iter).second->_sessionID);
			}
		}
		ReleaseSRWLockExclusive(&_mapLock);
		
		for (auto ID : v)
		{
			wcout << "No Packet long time , ID : " << ID << endl;
			Disconnect(ID);
		}
	}
}

VOID CLanMonitorServer::OnShutDown()
{

}

void CLanMonitorServer::HandlePacket_ServerLogin(ServerInfo* info, CPacket* packet)
{
	int serverNo;
	(*packet) >> serverNo;

	info->_serverFlag = true;
	info->_serverNo = serverNo;

	wcout << "Server Login, ServerNo : " << serverNo << endl;
}

void CLanMonitorServer::HandlePacket_DataUpdate(ServerInfo* info, CPacket* packet)
{
	BYTE dataType;
	int  dataValue;
	int  timeStamp;

	(*packet) >> dataType;
	(*packet) >> dataValue;
	(*packet) >> timeStamp;

	if (dataType < 0 || dataType >= dfMONITOR_DATA_NONE)
	{
		// �߸��� Ÿ��
		Log(L"Invalid dataType error");
		DebugBreak();
	}

	// ���� ����
	MonitorInfo* monitorInfo = _monitorInfoMap[dataType];
	AcquireSRWLockExclusive(&monitorInfo->_lock);
	monitorInfo->_serverNo = info->_serverNo;
	monitorInfo->update(dataValue);
	ReleaseSRWLockExclusive(&monitorInfo->_lock);

	// ����͸� Ŭ���̾�Ʈ�� �۽�
	CPacket* updatePacket = CPacket::Alloc();
	CreatePacket_DataUpdate(updatePacket, info->_serverNo, dataType, dataValue, timeStamp);
	_netServer->SendToMonitor(updatePacket);
	CPacket::Free(updatePacket);

	static int sum = 0;
	sum += 1;
	wcout << "Send Data " << dataType << " sum : " << sum << "\n";
}

void CLanMonitorServer::CreatePacket_DataUpdate(CPacket* packet, BYTE serverNo, BYTE dataType, int dataValue, int timeStamp)
{
	(*packet) << (WORD)en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE;
	(*packet) << serverNo;
	(*packet) << dataType;
	(*packet) << dataValue;
	(*packet) << timeStamp;
}

void CLanMonitorServer::DBFunc()
{	
	for (;;)
	{
		Sleep(60000);

		for (unordered_map<INT64, MonitorInfo*>::iterator iter = _monitorInfoMap.begin();
			iter != _monitorInfoMap.end(); iter++)
		{
			int type = (*iter).first;										// Type
			MonitorInfo* monitorInfo = (*iter).second;
			int serverNo = monitorInfo->_serverNo;							// Serverno
			if (serverNo == -1 || monitorInfo->_cnt == 0) 
				continue;

			AcquireSRWLockExclusive(&monitorInfo->_lock);
			int avg(0), maxValue(0), minValue(0);							// avg, max, min
			monitorInfo->GetMonitorValue(&avg, &maxValue, &minValue);
			monitorInfo->Init();
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			// DB ����� ���� ����
			char yearMonth[7];
			char query[512];
			
			time_t now = time(0);
			struct tm ltm;
			localtime_s(&ltm, &now);
			strftime(yearMonth, sizeof(yearMonth), "%Y%m", &ltm);

			snprintf(query, sizeof(query),
				"INSERT INTO monitorlog_%s (logtime, serverno, type, avg, max, min) VALUES (NOW(), %d, %d, %d, %d, %d);",
				yearMonth, serverNo, type, avg, maxValue, minValue);

			// ���� ���� (�ϴ� ���̺��� �����Ǽ� �ּ�ó��)
			// _logDB->ExecQuery(query);
		}

		// wcout << "DB Save" << endl;
	}
}

void CLanMonitorServer::MontiorFunc()
{
	for (;;)
	{
		Sleep(1000);

		_monitor.UpdateCpuTime();
		BYTE serverNo = 2;

		// 1. CPU ��ü ����
		{
			MonitorInfo* monitorInfo = _monitorInfoMap[dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL];
			int dataValue = (int)_monitor.ProcessorTotal();
			AcquireSRWLockExclusive(&monitorInfo->_lock);
			monitorInfo->_serverNo = serverNo;
			monitorInfo->update(dataValue);
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			CPacket* packet = CPacket::Alloc();
			CreatePacket_DataUpdate(
				packet,
				serverNo,
				dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL,
				dataValue,
				(int)time(nullptr));
			_netServer->SendToMonitor(packet);
			CPacket::Free(packet);
		}

		// 2. �������� �޸� MB
		{
			MonitorInfo* monitorInfo = _monitorInfoMap[dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY];
			int dataValue = ((int)_monitor.NonpagedMemory() / 1048576);
			AcquireSRWLockExclusive(&monitorInfo->_lock);
			monitorInfo->_serverNo = serverNo;
			monitorInfo->update(dataValue);
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			CPacket* packet = CPacket::Alloc();
			CreatePacket_DataUpdate(
				packet,
				serverNo,
				dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY,
				dataValue,
				(int)time(nullptr));
			_netServer->SendToMonitor(packet);
			CPacket::Free(packet);
		}

		// 3. ��Ʈ��ũ ���ŷ� KB
		{
			MonitorInfo* monitorInfo = _monitorInfoMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV];
			int dataValue = (int)_monitor.NetworkRecvMemory() / 1024;
			AcquireSRWLockExclusive(&monitorInfo->_lock);
			monitorInfo->_serverNo = serverNo;
			monitorInfo->update(dataValue);
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			CPacket* packet = CPacket::Alloc();
			CreatePacket_DataUpdate(
				packet,
				serverNo,
				dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV,
				dataValue,
				(int)time(nullptr));
			_netServer->SendToMonitor(packet);
			CPacket::Free(packet);
		}

		// 4. ��Ʈ��ũ �۽ŷ� KB
		{
			MonitorInfo* monitorInfo = _monitorInfoMap[dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND];
			int dataValue = (int)_monitor.NetworkSendMemory() / 1024;
			AcquireSRWLockExclusive(&monitorInfo->_lock);
			monitorInfo->_serverNo = serverNo;
			monitorInfo->update(dataValue);
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			CPacket* packet = CPacket::Alloc();
			CreatePacket_DataUpdate(
				packet,
				serverNo,
				dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND,
				dataValue,
				(int)time(nullptr));
			_netServer->SendToMonitor(packet);
			CPacket::Free(packet);
		}

		// 5. ��밡�� �޸�
		{
			MonitorInfo* monitorInfo = _monitorInfoMap[dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY];
			int dataValue = (int)_monitor.AvailableMemory();
			AcquireSRWLockExclusive(&monitorInfo->_lock);
			monitorInfo->_serverNo = serverNo;
			monitorInfo->update(dataValue);
			ReleaseSRWLockExclusive(&monitorInfo->_lock);

			CPacket* packet = CPacket::Alloc();
			CreatePacket_DataUpdate(
				packet,
				serverNo,
				dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY,
				dataValue,
				(int)time(nullptr));
			_netServer->SendToMonitor(packet);
			CPacket::Free(packet);
		}
	}
}

void DBThreadFunc(CLanMonitorServer* server)
{
	server->DBFunc();
}

void MonitorThreadFunc(CLanMonitorServer* server)
{
	server->MontiorFunc();
}
