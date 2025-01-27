#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <ws2tcpip.h>

const UINT16 PACKET_ID_SIZE = 28; // Last Packet_ID Num + 1

struct DataPacket {
	UINT32 dataSize;
	SOCKET userSkt;
	DataPacket(UINT32 dataSize_,SOCKET userSkt_) : dataSize(dataSize_), userSkt(userSkt_) {}
};

struct PacketInfo
{
	UINT16 packetId = 0;
	UINT16 dataSize = 0;
	SOCKET userSkt = 0;
	char* pData = nullptr;
};

struct PACKET_HEADER
{
	UINT16 PacketLength;
	UINT16 PacketId; 
	std::string uuid; // User UUID
};

enum class PACKET_ID : UINT16{
	//SYSTEM
	LOGIN = 1,
	LOGOUT = 2,
	USER_DISCONNECT = 3,
	SERVER_END = 4,

	// USER STATUS
	LEVEL_UP = 11,
	LEVEL_DOWN = 12,
	EXP_UP = 13,
	EXP_DOWN = 14,
	HP_UP = 15,
	HP_DOWN = 16,
	MP_UP = 17,
	MP_DOWN = 18,

	// INVENTORY
	ADDITEM = 25,
	DELITEM = 26,
	MOVEITEM = 27
};