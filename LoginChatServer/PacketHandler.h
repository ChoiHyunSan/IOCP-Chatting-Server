#pragma once
#include "pch.h"
#include "CServer.h"
#include "Type.h"
#include "ChatServer.h"

/******************************

		Packet Handler

******************************/
// - 패킷 핸들을 담당한다.

class PacketHandler
{
public:
	static void HandlePacket(const WORD type, User* user, CPacket* packet);

	static void HandlePacket_Login(User* user, CPacket* packet);
	static void HandlePacket_Move(User* user, CPacket* packet);
	static void HandlePacket_Message(User* user, CPacket* packet);
	static void HandlePacket_Heartbeat(User* user, CPacket* packet);

	static void ParsePacket_Login(CPacket* packet, out INT64* accountNo, out char* ID, out char* nickName, out char* sessionKey);
	static void ParsePacket_Move(CPacket* packet, out INT64& accountNo, out WORD& newX, out WORD& newY);
	static void ParsePacket_Message(CPacket* packet, out INT64& accountNo, out WORD& msgLen);

	static void CreatePacket_Login(CPacket* packet, const BYTE status, const INT64 accountNo);
	static void CreatePacket_Move(CPacket* packet, const INT64 accountNo, const WORD sX, const WORD sY);
	static void CreatePacket_Message(CPacket* packet, const INT64 accountNo, char* ID, char* nickName, char* msg, WORD msgLen);
};
