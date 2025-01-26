#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <ws2tcpip.h>

struct DataPacket {
	UINT32 dataSize;
	SOCKET userSkt;
	DataPacket(UINT32 dataSize_,SOCKET userSkt_) : dataSize(dataSize_), userSkt(userSkt_) {}
};

struct PacketInfo
{
	SOCKET UserSkt = 0;
	UINT16 PacketId = 0;
	UINT16 DataSize = 0;
	char* pDataPtr = nullptr;
};

struct PACKET_HEADER
{
	UINT16 PacketLength;
	UINT16 PacketId; 
	std::string uuid; // User UUID
};

enum class Packet_Id : UINT16{
	//SYSTEM
	USER_CONNECT = 11,
	USER_DISCONNECT = 12,

	// USER STATUS
	LEVEL_UP,
	LEVEL_DOWN,
	EXP_UP,
	EXP_DOWN,
	HP_UP,
	HP_DOWN,
	MP_UP,
	MP_DOWN,

	// INVENTORY
	
};