#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

const UINT32 MAX_SOCK = 1024; // Set Max Socket Buf
const UINT32 MAX_RECV_DATA = 8096;

const short MAX_RETRY_COUNT = 3;

enum class TaskType {
	ACCEPT,
	RECV,
	SEND
};

struct OverlappedEx {
	// 2 bytes
	short retryCnt = 0; // Retry Count For Send Proc

	// 4 bytes
	TaskType taskType; // ACCPET, RECV, SEND INFO

	// 8 bytes
	SOCKET userSkt;

	// 16 bytes
	WSABUF wsaBuf; // WSASend, WSARecv에 필요한 버퍼

	WSAOVERLAPPED wsaOverlapped;
};