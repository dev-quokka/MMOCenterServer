#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <ws2tcpip.h>

const UINT16 PACKET_ID_SIZE = 33; // Last Packet_ID Num + 1

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
	std::string uuId; // UUID For User Check
};

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	UINT16 userPk;
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	char* userInfo = nullptr;
	char* inventory = nullptr;
};

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t slotPos; // (Max 50)
	short itemCount; // (Max 999)
	short itemCode; // (Max 5000)
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {

};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t slotPos; // (Max 50)
	short itemCode; // (Max 5000)
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {

};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint8_t dragitemType; // (Max 3)
	uint8_t dragslotPos; // (Max 50)
	int8_t targetitemType; // (Max 3)
	uint8_t targetslotPos; // (Max 50)
	short dragitemCount; // (Max 999)
	short dragitemCode; // (Max 5000)
	short targetitemCount; // (Max 999)
	short targetitemCode; // (Max 5000)
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {

};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t slotPos; // (Max 50)
	short itemCount; // (Max 999)
	short itemCode; // (Max 5000)
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {

};

enum class PACKET_ID : UINT16{
	//SYSTEM
	USER_CONNECT = 1,
	LOGOUT = 2,
	USER_DISCONNECT = 3,
	SERVER_END = 4,

	// USER STATUS (11~)
	USER_CONNECT_REQUEST = 11,
	USER_CONNECT_RESPONSE = 12,
	USER_DISCONNECT_REQUEST = 13,
	USER_DISCONNECT_RESPONSE = 14,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25,
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,
	DEL_ITEM_RESPONSE = 28,
	MOV_ITEM_REQUEST = 29,
	MOV_ITEM_RESPONSE = 30,
	MOD_ITEM_REQUEST = 31,
	MOD_ITEM_RESPONSE = 32
};