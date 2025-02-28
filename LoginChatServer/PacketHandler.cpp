#include "PacketHandler.h"

void PacketHandler::HandlePacket(const WORD type, User* user, CPacket* packet)
{
	switch (type)
	{
		case dfJOB_LOGIN:
			HandlePacket_Login(user, packet);
			break;
		case dfJOB_MOVE:
			HandlePacket_Move(user, packet);
			break;
		case dfJOB_Message:
			HandlePacket_Message(user, packet);
			break;
		case dfJOB_HEARTBEAT:
			HandlePacket_Heartbeat(user, packet);
			break;
		default:
			g_server.Disconnect(user->_sessionID);
			break;
	}
}

void PacketHandler::HandlePacket_Login(User* user, CPacket* packet)
{
	ParsePacket_Login(packet, &user->_accountNo, 
		reinterpret_cast<char*>(user->_ID), 
		reinterpret_cast<char*>(user->_NickName), 
		user->_sessionKey);

	user->_redisFuture = g_server._redisClient.get(to_string(user->_accountNo));
	g_server._redisClient.commit();  

	g_server.RequestCheckID(user->_sessionID);
}

void PacketHandler::HandlePacket_Move(User* user, CPacket* packet)
{
	// Step 1. Parsing Packet
	INT64 accountNo;
	WORD newX, newY;
	ParsePacket_Move(packet, accountNo, newX, newY);

	// Step 2. Move User
	g_server.MoveUser(newX, newY, user);

	// Step 3. Response Packet
	CPacket* movePacket = CPacket::Alloc();
	CreatePacket_Move(movePacket, accountNo, newX, newY);
	g_server.SendUnicast(movePacket, user->_sessionID);
	CPacket::Free(movePacket);
}

void PacketHandler::HandlePacket_Message(User* user, CPacket* packet)
{
	// Step 1. Parsing Packet
	INT64  accountNo;
	WORD   msgLen;
	ParsePacket_Message(packet, accountNo, msgLen);

	// Step 2. Send Message & Responce Packet
	CPacket* msgPacket = CPacket::Alloc();
	CreatePacket_Message(msgPacket, accountNo, reinterpret_cast<char*>(user->_ID), reinterpret_cast<char*>(user->_NickName), packet->GetReadPtr(), msgLen);
	g_server.SendAroundSector(msgPacket, user);
	CPacket::Free(msgPacket);
}

void PacketHandler::HandlePacket_Heartbeat(User* user, CPacket* packet)
{
	user->_prevHeartBeat = GetTickCount64();
}

void PacketHandler::ParsePacket_Login(CPacket* packet, out INT64* accountNo, out char* ID, out char* nickName, out char* sessionKey)
{
	(*packet) >> *accountNo;
	(*packet).GetData(ID, sizeof(WCHAR) * 20);
	(*packet).GetData(nickName, sizeof(WCHAR) * 20);
	(*packet).GetData(sessionKey, 64);
}

void PacketHandler::ParsePacket_Move(CPacket* packet, out INT64& accountNo, out WORD& newX, out WORD& newY)
{
	(*packet) >> accountNo >> newX >> newY;
}

void PacketHandler::ParsePacket_Message(CPacket* packet, out INT64& accountNo, out WORD& msgLen)
{
	(*packet) >> accountNo >> msgLen;
}

void PacketHandler::CreatePacket_Login(CPacket* packet, const BYTE status, const INT64 accountNo)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_LOGIN;
	(*packet) << (BYTE)status;
	(*packet) << accountNo;
}

void PacketHandler::CreatePacket_Move(CPacket* packet, const INT64 accountNo, const WORD sX, const WORD sY)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_SECTOR_MOVE;
	(*packet) << accountNo;
	(*packet) << sX;
	(*packet) << sY;
}

void PacketHandler::CreatePacket_Message(CPacket* packet, const INT64 accountNo, char* ID, char* nickName, char* msg, WORD msgLen)
{
	(*packet) << (WORD)en_PACKET_CS_CHAT_RES_MESSAGE;
	(*packet) << accountNo;
	(*packet).PutData(ID, sizeof(WCHAR) * 20);
	(*packet).PutData(nickName, sizeof(WCHAR) * 20);
	(*packet) << msgLen;
	(*packet).PutData(msg, msgLen);
}
