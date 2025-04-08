#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>
#include <string>
#include <unordered_map>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max RECV Buf
const uint32_t MAX_CIRCLE_SIZE = 8096;

const short MAX_RETRY_COUNT = 3;

std::string JWT_SECRET = "Cute_Quokka";

// ---------------------------- MYSQL  ----------------------------
//CenterServer = 1
//ChannelServer01 = 10~, 1~
//ChannelServer02 = 20~, 2~



//  ---------------------------- SERVER INFO  ----------------------------

enum class ServerType : uint16_t{ // �߾� ������ ����ϴ� ��ȣ
	// Server Type (1~)
	GatewayServer = 1,
	MatchingServer = 2,

	// Channel Server (11~)
	ChannelServer11 = 11,
	ChannelServer12 = 12,
	ChannelServer13 = 13,

	ChannelServer21 = 21,
	ChannelServer22 = 22,
	ChannelServer23 = 23,

	// Game Server (51~)
	RaidGameServer01 = 31,
};

enum class ChannelType : uint16_t {
	CH_11 = 1, // 1-1����
	CH_12 = 2, // 1-2����
	CH_13 = 3, // 1-3����

	CH_21 = 4, // 2-1����
	CH_22 = 5, // 2-2����
	CH_23 = 6, // 2-3����
};

struct ServerAddress {
	std::string ip;
	uint16_t port;
};

std::unordered_map<ServerType, ServerAddress> ServerAddressMap;


//  ---------------------------- SYSTEM  ----------------------------

enum class TaskType {
	ACCEPT,
	RECV,
	SEND,
	NEWRECV, // ������ Ǯ �� �Ἥ ���� ���� ����Ѱ�. (�̰� �� ���� ����)
	NEWSEND
};

struct OverlappedEx {
	WSAOVERLAPPED wsaOverlapped;
	// 4 bytes
	TaskType taskType; // ACCPET, RECV, SEND INFO
};

struct OverlappedTCP : OverlappedEx {
	// 2 bytes
	uint16_t connObjNum;
	// 16 bytes
	WSABUF wsaBuf; // TCP Buffer
};

