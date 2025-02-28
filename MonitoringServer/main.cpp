#include "CMonitorServer.h"
#include "CLanMonitorServer.h"

void LaunchServer(CServer* server)
{
	server->Launch();
}

int wmain()
{
	CMonitorServer		NetServer;
	CLanMonitorServer	LanServer(&NetServer, L"LanServerSetting.txt");

	vector<thread> _server;
	_server.push_back(thread(LaunchServer, &NetServer));
	_server.push_back(thread(LaunchServer, &LanServer));

	for (auto& thread : _server)
	{
		if (thread.joinable())
			thread.join();
	}

	return 0;
}
