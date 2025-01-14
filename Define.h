#pragma once
#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

const UINT32 MAX_SOCK = 1024; // Set Max Socket Buf

enum class TaskType {
	ACCEPT,
	RECV,
	SEND
};

struct OverlappedEx {
	TaskType taskType; // ACCPET, RECV, SEND INFO
	UINT32 UserIdx; // User Idx for distinguish user
	WSABUF wsaBuf; // WSASend, WSARecv에 필요한 버퍼
	WSAOVERLAPPED wsaOverlapped;
};