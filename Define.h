#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <cstdint>

const uint32_t MAX_RECV_SIZE = 1024; // Set Max RECV Buf
const uint32_t MAX_CIRCLE_SIZE = 8096;

const short MAX_RETRY_COUNT = 3;

enum class TaskType {
	ACCEPT,
	RECV,
	SEND
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

struct OverlappedUDP : OverlappedEx {
	// 4 bytes
	int addrSize = sizeof(sockaddr_in);

	// 16 bytes
	WSABUF wsaBuf; // UDP Buffer
	sockaddr_in userAddr;  // Client Ip && Port Info
};

