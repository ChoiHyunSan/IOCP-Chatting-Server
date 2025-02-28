#pragma once
#include "pch.h"
#include "CServer.h"
#include "Type.h"

/******************************

		Packet Handler

******************************/
// - 패킷 핸들을 담당한다.

class PacketHandler
{
public:
	static void HandlePacket(const WORD type, User* user, CPacket* packet);

	static void HandlePacket_Login(User* user, CPacket* packet);
	
	static void ParsePacket_Login(CPacket* packet, out INT64* accountNo, out char* sessionKey);

	static void CreatePacket_Login(CPacket* packet, const BYTE status, const INT64 accountNo, char* ID, char* Nickname, char* GameServer, USHORT GameServerPort, char* ChatServer, USHORT ChatServerPort);
};
