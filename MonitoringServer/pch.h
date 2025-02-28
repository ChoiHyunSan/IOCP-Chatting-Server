#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment (lib, "cpp_redis.lib")
#pragma comment (lib, "tacopie.lib")
#pragma comment (lib, "ws2_32.lib")
#pragma comment(lib, "mysqlclient.lib")

#pragma comment(lib, "ws2_32")
#include <process.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <unordered_map>
#include <queue>
#include <iomanip>

#include "include\\mysql.h"
#include "include\\errmsg.h"
#include "CRingBuffer.h"
#include "WParsor.h"
#include "Define.h"
#include "CPacket.h"
#include "Log.h"
#include "CProfiler.h"
#include "CrashDump.h"
#include "ObjectPool.h"
#include "CLockFreeQueue.h"
#include "CEncryption.h"
#include "CMonitor.h"
#include "CParsor.h"
#include "SQL.h"

#include <thread>

using namespace std;