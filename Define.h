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
	// 4 bytes
	TaskType taskType; // ACCPET, RECV, SEND INFO
	UINT32 UserIdx; // User Idx for distinguish user

	// 16 bytes
	WSABUF wsaBuf; // WSASend, WSARecv�� �ʿ��� ����

	WSAOVERLAPPED wsaOverlapped;
};

struct AcceptInfo {
	// 4 bytes
	UINT32 a_Idx;
	
	// 8 bytes
	SOCKET a_Socket;

	// 56 bytes
	OverlappedEx a_OvLap;
};