#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const uint16_t RANKING_USER_COUNT = 3; // ��� ���� ��ŷ ���� �����ð���

const int MAX_USER_ID_LEN = 32;
const int MAX_SERVER_USERS = 128; // ���� ���� �� ���� ��Ŷ
const int MAX_JWT_TOKEN_LEN = 256;
const int MAX_SCORE_SIZE = 512;

struct DataPacket {
	uint32_t dataSize;
	uint16_t connObjNum;
	DataPacket(uint32_t dataSize_, uint16_t connObjNum_) : dataSize(dataSize_), connObjNum(connObjNum_) {}
	DataPacket() = default;
};

struct PacketInfo
{
	uint16_t packetId = 0;
	uint16_t dataSize = 0;
	uint16_t connObjNum = 0;
	char* pData = nullptr;
};

struct PACKET_HEADER
{
	uint16_t PacketLength;
	uint16_t PacketId;
};

struct RANKING {
	uint16_t score = 0;
	char userId[MAX_USER_ID_LEN + 1] = {};
};

//  ---------------------------- SYSTEM  ----------------------------

struct USER_CONNECT_REQUEST_PACKET : PACKET_HEADER {
	char userId[MAX_USER_ID_LEN + 1];
	char userToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
};

struct USER_CONNECT_RESPONSE_PACKET : PACKET_HEADER {
	bool isSuccess;
};

struct USER_LOGOUT_REQUEST_PACKET : PACKET_HEADER {

};

struct IM_SESSION_REQUEST : PACKET_HEADER {
	char Token[MAX_JWT_TOKEN_LEN + 1]; // Token For Session Server Check
};

struct IM_SESSION_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct SYNCRONIZE_LEVEL_REQUEST : PACKET_HEADER {
	uint16_t level;
	uint16_t userPk;
	unsigned int currentExp;
};

struct SYNCRONIZE_LOGOUT_REQUEST : PACKET_HEADER {
	uint16_t userPk;
};

struct SERVER_USER_COUNTS_REQUEST : PACKET_HEADER {

};

struct SERVER_USER_COUNTS_RESPONSE : PACKET_HEADER {
	char serverUserCnt[MAX_SERVER_USERS + 1];
	uint16_t serverCount; 
};

struct MOVE_SERVER_REQUEST : PACKET_HEADER {
	std::string channelName;
};

struct MOVE_SERVER_RESPONSE : PACKET_HEADER {
	std::string token;
	std::string ip;
	uint16_t port;
};

//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	uint16_t increaseLevel;
	unsigned int currentExp;
};

//  ---------------------------- INVENTORY  ----------------------------

struct ADD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	uint16_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
};

struct DEL_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOD_ITEM_REQUEST : PACKET_HEADER {
	uint16_t itemType; // (Max 3)
	uint16_t itemPosition; // (Max 50)
	int8_t itemCount; // (Max 99)
	uint16_t itemCode; // (Max 5000)
};

struct MOD_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct MOV_ITEM_REQUEST : PACKET_HEADER {
	uint16_t ItemType; // (Max 3)

	uint16_t dragItemPos; // (Max 10)
	uint16_t dragItemCode;
	uint16_t dragItemCount; // (Max 99)

	uint16_t targetItemPos; // (Max 10)
	uint16_t targetItemCode;
	uint16_t targetItemCount; // (Max 99)
};

struct MOV_ITEM_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

struct ADD_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
	uint16_t Enhancement; // (Max 20)
	uint16_t itemCode; // (Max 5000)
};

struct ADD_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct DEL_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct DEL_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};

struct ENH_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t itemPosition; // (Max 50)
};

struct ENH_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
	uint16_t Enhancement = 0;
};

struct MOV_EQUIPMENT_REQUEST : PACKET_HEADER {
	uint16_t dragItemPos; // (Max 10)
	uint16_t dragItemCode;
	uint16_t dragItemEnhancement;

	uint16_t targetItemPos; // (Max 10)
	uint16_t targetItemCode;
	uint16_t targetItemEnhancement;
};

struct MOV_EQUIPMENT_RESPONSE : PACKET_HEADER {
	bool isSuccess;
};


//  ---------------------------- RAID  ----------------------------

struct RAID_MATCHING_REQUEST : PACKET_HEADER { // Users Matching Request

};

struct RAID_MATCHING_RESPONSE : PACKET_HEADER {
	bool insertSuccess; // Insert Into Matching Queue Check
};

struct RAID_READY_REQUEST : PACKET_HEADER {
	std::string ip;
	uint16_t port;
	uint16_t udpPort;
	uint16_t roomNum;
};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	char reqScore[MAX_SCORE_SIZE + 1];
	uint16_t rkCount;
};


//  ---------------------------- Matching Server  ----------------------------

struct MATCHING_REQUEST : PACKET_HEADER {
	uint16_t userObjNum;
	uint16_t userGroupNum;
};

struct MATCHING_RESPONSE : PACKET_HEADER {
	uint16_t userObjNum;
	bool isSuccess;
};

struct MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t userObjNum1;
	uint16_t userObjNum2;
};

struct RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER : PACKET_HEADER { // �������� ��Ī ������ ����
	uint16_t roomNum;
};

enum class SESSION_ID : uint16_t {
	// SYSTEM (1~)
	IM_SESSION_REQUEST = 1,
	IM_SESSION_RESPONSE = 2,

	// USER LOGIN (11~)
	USER_LOGIN_REQUEST = 11,
	USER_LOGIN_RESPONSE = 12,
	USER_GAMESTART_REQUEST = 13,
	USER_GAMESTART_RESPONSE = 14,
	USERINFO_REQUEST = 15,
	USERINFO_RESPONSE = 16,
	EQUIPMENT_REQUEST = 17,
	EQUIPMENT_RESPONSE = 18,
	CONSUMABLES_REQUEST = 19,
	CONSUMABLES_RESPONSE = 20,
	MATERIALS_REQUEST = 21,
	MATERIALS_RESPONSE = 22,

	// SYNCRONIZATION (51~)
	SYNCRONIZE_LEVEL_REQUEST = 51,
	SYNCRONIZE_LOGOUT_REQUEST = 52,
	SYNCRONIZE_DISCONNECT_REQUEST = 53,

};

enum class CHANNEL_ID : uint16_t {
	// SYSTEM (1~)
	IM_CHANNEL1_REQUEST = 1,
	IM_CHANNEL1_RESPONSE = 2,


	// USER STATUS (21~)
	EXP_UP_REQUEST = 21,
	EXP_UP_RESPONSE = 22,
	LEVEL_UP_REQUEST = 23,
	LEVEL_UP_RESPONSE = 24,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25,
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,
	DEL_ITEM_RESPONSE = 28,
	MOD_ITEM_REQUEST = 29,
	MOD_ITEM_RESPONSE = 30,
	MOV_ITEM_REQUEST = 31,
	MOV_ITEM_RESPONSE = 32,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 33,
	ADD_EQUIPMENT_RESPONSE = 34,
	DEL_EQUIPMENT_REQUEST = 35,
	DEL_EQUIPMENT_RESPONSE = 36,
	ENH_EQUIPMENT_REQUEST = 37,
	ENH_EQUIPMENT_RESPONSE = 38,
	MOV_EQUIPMENT_REQUEST = 39,
	MOV_EQUIPMENT_RESPONSE = 40,


	// RAID (55~)
	RAID_RANKING_REQUEST = 55,
	RAID_RANKING_RESPONSE = 56,
};

enum class MATCHING_ID : uint16_t {
	//SYSTEM
	IM_MATCHING_REQUEST = 1,
	IM_MATCHING_RESPONSE = 2, 

	//RAID(11~)
	MATCHING_REQUEST = 11,
	MATCHING_RESPONSE = 12,
	MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER = 13,
	RAID_START_FAIL_REQUEST_TO_MATCHING_SERVER = 14
};

enum class PACKET_ID : uint16_t {
	// SYSTEM (1~)
	USER_CONNECT_REQUEST = 1,
	USER_CONNECT_RESPONSE = 2,
	USER_LOGOUT_REQUEST = 3, 
	USER_FULL_REQUEST = 6, 
	WAITTING_NUMBER_REQUSET = 7,
	SERVER_USER_COUNTS_REQUEST = 8,
	SERVER_USER_COUNTS_RESPONSE = 9,
	MOVE_SERVER_REQUEST = 10,
	MOVE_SERVER_RESPONSE = 11,

	// USER STATUS (21~)
	EXP_UP_REQUEST = 21, 
	EXP_UP_RESPONSE = 22,
	LEVEL_UP_REQUEST = 23,
	LEVEL_UP_RESPONSE = 24,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25, 
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,
	DEL_ITEM_RESPONSE = 28,
	MOD_ITEM_REQUEST = 29, 
	MOD_ITEM_RESPONSE = 30,
	MOV_ITEM_REQUEST = 31,  
	MOV_ITEM_RESPONSE = 32,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 33,
	ADD_EQUIPMENT_RESPONSE = 34,
	DEL_EQUIPMENT_REQUEST = 35,
	DEL_EQUIPMENT_RESPONSE = 36,
	ENH_EQUIPMENT_REQUEST = 37,
	ENH_EQUIPMENT_RESPONSE = 38,
	MOV_EQUIPMENT_REQUEST = 39,
	MOV_EQUIPMENT_RESPONSE = 40,

	// RAID (45~)
	RAID_MATCHING_REQUEST = 45, 
	RAID_MATCHING_RESPONSE = 46,
	RAID_READY_REQUEST = 47,
	RAID_RANKING_REQUEST = 55, 
	RAID_RANKING_RESPONSE = 56,
};