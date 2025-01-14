#pragma once

#include "Define.h"

#include <iostream>

class ConnUser {
public:
	ConnUser(SOCKET UserSkt_,UINT32 UserIdx_) : userSkt(UserSkt_), userIdx(UserIdx_) {}

public :
	bool IsConn() { // check connection status
		return isConn;
	}

	SOCKET GetSktNum() {
		return userSkt;
	}

	bool PrepareAccept(SOCKET ServerSkt_) {
		userOvlap.taskType = TaskType::ACCEPT;
		userOvlap.UserIdx = userIdx;
		userOvlap.wsaBuf.buf = nullptr;
		userOvlap.wsaBuf.len = 0;

		DWORD bytes = 0;
		DWORD flags = 0;

		if (AcceptEx(ServerSkt_, userSkt, AcceptBuf,0,sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN) + 16,&bytes,(LPWSAOVERLAPPED)&userOvlap )==0) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				std::cout << "AcceptEx Error : " << GetLastError() << std::endl;
				return false;
			}
		}

		else std::cout << "Accept request skt : " << userSkt << std::endl;

		return true;
	}

	bool BindUser() {
		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)userSkt, userIocpHandle, (ULONG_PTR)(this), 0);
		
		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort()함수 실패 :" << GetLastError() << std::endl;
			return false;
		}

		return true;
	}

	bool ConnUserRecv() {
		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		userOvlap = {};

		userOvlap.wsaBuf.len = MAX_SOCK;
		userOvlap.wsaBuf.buf = RecvBuf;
		userOvlap.taskType = TaskType::RECV;

		int tempR = WSARecv(userSkt,&(userOvlap.wsaBuf),1,&dwRecvBytes, &dwFlag,(LPWSAOVERLAPPED)&(userOvlap),NULL);
		
		//socket_error이면 client socket이 끊어진걸로 처리한다.
		if (tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << "WSARecv()함수 실패 : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;

	}

private:
	// 1 bytes
	bool isConn = 0;
	char AcceptBuf[64];
	char RecvBuf[MAX_SOCK];

	// 4 bytes
	UINT32 userIdx = 0;

	// 8 bytes
	SOCKET userSkt;
	HANDLE userIocpHandle = INVALID_HANDLE_VALUE;

	// 56 bytes
	OverlappedEx userOvlap = {};
};