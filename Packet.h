#pragma once
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>
#include <ws2tcpip.h>

const UINT16 PACKET_ID_SIZE = 39; // Last Packet_ID Num + 1

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

//  ---------------------------- SYSTEM  ----------------------------

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	uint8_t level;
	UINT16 userPk;
	unsigned int currentExp;
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
};

struct IM_WEB_REQUEST : PACKET_HEADER {

};

struct IM_WEB_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct SYNCRONIZE_LEVEL_REQUEST : PACKET_HEADER {
	uint8_t level;
	UINT16 userPk;
	unsigned int currentExp;
};

struct SYNCRONIZE_USERINFO_REQUEST : PACKET_HEADER {
	UINT16 userPk;
	// UserInfo
	// Inventory
};

//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	unsigned int expUp;
};

struct LEVEL_UP_RESPONSE : PACKET_HEADER {
	uint8_t increaseLevel;
	unsigned int currentExp;
};

//  ---------------------------- INVENTORY  ----------------------------

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	uint8_t itemCount; // (Max 99)
	short itemCode; // (Max 5000)
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	short itemCode; // (Max 5000)
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	int8_t itemCount; // (Max 99)
	short itemCode; // (Max 5000)
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint8_t dragItemType; // (Max 3)
	uint8_t targetItemType; // (Max 3)
	uint8_t dragItemSlotPos; // (Max 50)
	uint8_t targetItemSlotPos; // (Max 50)
	uint8_t dragItemCount; // (Max 99)
	uint8_t targetItemCount; // (Max 99)
	short dragItemCode; // (Max 5000)
	short targetItemCode; // (Max 5000)
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

struct ADD_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	uint8_t currentEnhanceCount; // (Max 20)
	short itemCode; // (Max 5000)
};

struct ADD_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	short itemCode; // (Max 5000)
};

struct DEL_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct ENH_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint8_t itemType; // (Max 3)
	uint8_t itemSlotPos; // (Max 50)
	uint8_t currentEnhanceCount; // (Max 10)
	short itemCode; // (Max 5000)
};

struct ENH_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

enum class PACKET_ID : UINT16 {
	//SYSTEM
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
	USER_LOGOUT_REQUEST = 3,
	IM_WEB_REQUEST = 4,
	IM_WEB_RESPONSE = 5,
	SYNCRONIZE_LEVEL_REQUEST = 6,

	// USER STATUS (11~)
	EXP_UP_REQUEST = 11,
	EXP_UP_RESPONSE = 12,
	LEVEL_UP_REQUEST = 13,
	LEVEL_UP_RESPONSE = 14,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25,
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,
	DEL_ITEM_RESPONSE = 28,
	MOD_ITEM_REQUEST = 29,
	MOD_ITEM_RESPONSE = 30,

	// COMMON
	MOV_ITEM_REQUEST = 31,
	MOV_ITEM_RESPONSE = 32,

	// EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 33,
	ADD_EQUIPMENT_RESPONSE = 34,
	DEL_EQUIPMENT_REQUEST = 35,
	DEL_EQUIPMENT_RESPONSE = 36,
	ENH_EQUIPMENT_REQUEST = 37,
	ENH_EQUIPMENT_RESPONSE = 38
};