#pragma once
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

const uint16_t RANKING_USER_COUNT = 3; // ��� ���� ��ŷ ���� �����ð���

const int MAX_USER_ID_LEN = 32;
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

struct IM_WEB_REQUEST : PACKET_HEADER {
	char webToken[MAX_JWT_TOKEN_LEN + 1]; // userToken For User Check
};

struct IM_WEB_RESPONSE : PACKET_HEADER {
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

//  ---------------------------- USER STATUS  ----------------------------

struct EXP_UP_REQUEST : PACKET_HEADER {
	short mobNum; // Number of Mob
};

struct EXP_UP_RESPONSE : PACKET_HEADER {
	uint16_t increaseLevel;
	unsigned int currentExp;
};

struct LEVEL_UP_RESPONSE : PACKET_HEADER {
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
	uint16_t itemCode; // (Max 5000)
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
	uint16_t timer; // Minutes
	uint16_t roomNum; // If Max RoomNum Up to Short Range, Back to Number One
	uint16_t yourNum;
	int mobHp;
};

struct RAID_TEAMINFO_REQUEST : PACKET_HEADER { // User To Server
	bool imReady;
	uint16_t roomNum;
	uint16_t myNum;
	sockaddr_in userAddr; // ������ ���� udp ������ sockaddr_in ����
};

struct RAID_TEAMINFO_RESPONSE : PACKET_HEADER {
	uint16_t teamLevel;
	char teamId[MAX_USER_ID_LEN + 1];
};

struct RAID_START_REQUEST : PACKET_HEADER {
	std::chrono::time_point<std::chrono::steady_clock> endTime;
};

struct RAID_HIT_REQUEST : PACKET_HEADER {
	uint16_t roomNum;
	uint16_t myNum;
	unsigned int damage;
};

struct RAID_HIT_RESPONSE : PACKET_HEADER {
	unsigned int yourScore;
	unsigned int currentMobHp;
};

struct RAID_END_REQUEST : PACKET_HEADER { // Server to USER
	unsigned int userScore;
	unsigned int teamScore;
};

struct RAID_END_RESPONSE : PACKET_HEADER { // User to Server (If Server Get This Packet, Return Room Number)

};

struct RAID_RANKING_REQUEST : PACKET_HEADER {
	uint16_t startRank;
};

struct RAID_RANKING_RESPONSE : PACKET_HEADER {
	uint16_t rkCount;
	char reqScore[MAX_SCORE_SIZE + 1];
};

enum class PACKET_ID : uint16_t {
	//SYSTEM
	USER_CONNECT_REQUEST = 1, // ������ 2������ ��û 
	USER_CONNECT_RESPONSE = 2,
	USER_LOGOUT_REQUEST = 3, // ������ 3������ ��û 
	IM_WEB_REQUEST = 4, // ������ 1������ ��û 
	IM_WEB_RESPONSE = 5,
	USER_FULL_REQUEST = 6, // SERVER TO USER
	WAITTING_NUMBER_REQUSET = 7, // SERVER TO USER

	// USER STATUS (21~)
	EXP_UP_REQUEST = 21,  // ������ 4������ ��û 
	EXP_UP_RESPONSE = 22,
	LEVEL_UP_REQUEST = 23,// SERVER TO USER
	LEVEL_UP_RESPONSE = 24,

	// INVENTORY (25~)
	ADD_ITEM_REQUEST = 25,  // ������ 5������ ��û 
	ADD_ITEM_RESPONSE = 26,
	DEL_ITEM_REQUEST = 27,  // ������ 6������ ��û 
	DEL_ITEM_RESPONSE = 28,
	MOD_ITEM_REQUEST = 29,  // ������ 7������ ��û 
	MOD_ITEM_RESPONSE = 30,
	MOV_ITEM_REQUEST = 31,  // ������ 8������ ��û 
	MOV_ITEM_RESPONSE = 32,

	// INVENTORY::EQUIPMENT 
	ADD_EQUIPMENT_REQUEST = 33,  // ������ 9������ ��û 
	ADD_EQUIPMENT_RESPONSE = 34,
	DEL_EQUIPMENT_REQUEST = 35,  // ������ 10������ ��û 
	DEL_EQUIPMENT_RESPONSE = 36,
	ENH_EQUIPMENT_REQUEST = 37,  // ������ 11������ ��û 
	ENH_EQUIPMENT_RESPONSE = 38,
	MOV_EQUIPMENT_REQUEST = 39,
	MOV_EQUIPMENT_RESPONSE = 40,

	// RAID (45~)
	RAID_MATCHING_REQUEST = 45,  // ������ 12������ ��û 
	RAID_MATCHING_RESPONSE = 46,
	RAID_READY_REQUEST = 47,
	RAID_TEAMINFO_REQUEST = 48,  // ������ 13������ ��û 
	RAID_TEAMINFO_RESPONSE = 49,
	RAID_START_REQUEST = 50,
	RAID_HIT_REQUEST = 51,  // ������ 14������ ��û 
	RAID_HIT_RESPONSE = 52,
	RAID_END_REQUEST = 53,  // ������ 15������ ��û , �����δ� 1������ ��û
	RAID_END_RESPONSE = 54,
	RAID_RANKING_REQUEST = 55, // ������ 16������ ��û 
	RAID_RANKING_RESPONSE = 56,

	// WebServer Syncronizing Packet Id (101~)
	SYNCRONIZE_LEVEL_REQUEST = 101, // SERVER TO WEB SERVER
	SYNCRONIZE_LOGOUT_REQUEST = 102, // SERVER TO WEB SERVER
	SYNCRONIZE_DISCONNECT_REQUEST = 103, // SERVER TO WEB SERVER
};