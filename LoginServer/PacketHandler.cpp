#include "PacketHandler.h"
#include "CLoginServer.h"

void PacketHandler::HandlePacket(const WORD type, User* user, CPacket* packet)
{
	switch (type)
	{
	case dfJOB_LOGIN:
		HandlePacket_Login(user, packet);
		break;
	default:
		// 예외처리
		g_server.Disconnect(user->_sessionID);
		break;
	}
}

void PacketHandler::HandlePacket_Login(User* user, CPacket* packet)
{
	// Step 1. Parsing Packet
	ParsePacket_Login(packet, &user->_accountNo,  user->_sessionKey);

	// Step 2. DB 인증요청
	g_server._dbQueue.Enqueue(user->_sessionID);
	SetEvent(g_server._dbEvent);
}

void PacketHandler::ParsePacket_Login(CPacket* packet, out INT64* accountNo, out char* sessionKey)
{
	(*packet) >> *accountNo;
	(*packet).GetData(sessionKey, 64);
}

void PacketHandler::CreatePacket_Login(CPacket* packet, const BYTE status, const INT64 accountNo, char* ID, char* Nickname, char* GameServer, USHORT GameServerPort, char* ChatServer, USHORT ChatServerPort)
{
	(*packet) << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
	(*packet) << accountNo;
	(*packet) << (BYTE)status;
	(*packet).PutData(ID, sizeof(WCHAR) * 20);
	(*packet).PutData(Nickname, sizeof(WCHAR) * 20);
	(*packet).PutData(GameServer, sizeof(WCHAR) * 16);
	(*packet) << GameServerPort;
	(*packet).PutData(ChatServer, sizeof(WCHAR) * 16);
	(*packet) << ChatServerPort;
}
