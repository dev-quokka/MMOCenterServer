#pragma once

#include "Define.h"
#include "CircularBuffer.h"
#include "Packet.h"

#include <iostream>
#include <boost/lockfree/queue.hpp>

class ConnUser {
public:
	ConnUser(SOCKET UserSkt_, UINT32 bufferSize_, UINT16 connObjNum_, HANDLE sIOCPHandle) : userSkt(UserSkt_), circularBuffer(bufferSize_), connObjNum(connObjNum_), userIocpHandle(sIOCPHandle) {}

public :
	bool IsConn() { // check connection status
		return isConn;
	}

	char* GetRecvBuffer() {
		return recvBuf;
	}

	UINT16 GetObjNum() {
		return connObjNum;
	}

	bool WriteRecvData(const char* data_, UINT32 size_) {
		return circularBuffer.Write(data_,size_);
	}

	PacketInfo ReadRecvData(char* readData_, UINT32 size_) {
		if (circularBuffer.Read(readData_, size_)) {
			auto pHeader = (PACKET_HEADER*)readData_;

			PacketInfo packetInfo;
			packetInfo.packetId = pHeader->PacketId;
			packetInfo.dataSize = pHeader->PacketLength;
			packetInfo.userSkt = userSkt;
			packetInfo.pData = readData_;

			return packetInfo;
		}
	}

	void Reset() {
		isConn = false;
		memset(acceptBuf, 0, sizeof(acceptBuf));
		memset(recvBuf, 0, sizeof(recvBuf));
		userOvlap = {};
		userIocpHandle = INVALID_HANDLE_VALUE;
	}

	bool PostAccept(SOCKET ServerSkt_) {
		userOvlap.taskType = TaskType::ACCEPT;
		userOvlap.userSkt = userSkt;
		userOvlap.wsaBuf.buf = nullptr;
		userOvlap.wsaBuf.len = 0;

		DWORD bytes = 0;
		DWORD flags = 0;

		if (AcceptEx(ServerSkt_, userSkt, acceptBuf,0,sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN) + 16,&bytes,(LPWSAOVERLAPPED)&userOvlap )==0) {
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
			std::cout << "reateIoCompletionPort()�Լ� ���� :" << GetLastError() << std::endl;
			return false;
		}

		return ConnUserRecv();
	}
	
	bool ConnUserRecv() {
		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		userOvlap = {};

		userOvlap.wsaBuf.len = MAX_SOCK;
		userOvlap.wsaBuf.buf = recvBuf;
		userOvlap.userSkt = userSkt;
		userOvlap.taskType = TaskType::RECV;

		int tempR = WSARecv(userSkt,&(userOvlap.wsaBuf),1,&dwRecvBytes, &dwFlag,(LPWSAOVERLAPPED)&(userOvlap),NULL);
		
		if (tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << userSkt << " WSARecv() Fail : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	void PushSendMsg(const UINT32 dataSize_, char* sendMsg) {
		auto sendOverlapped = new OverlappedTCP;
		ZeroMemory(sendOverlapped, sizeof(OverlappedTCP));
		sendOverlapped->wsaBuf.len = dataSize_;
		sendOverlapped->wsaBuf.buf = new char[dataSize_];
		CopyMemory(sendOverlapped->wsaBuf.buf, sendMsg, dataSize_);
		sendOverlapped->taskType = TaskType::SEND;

		sendQueue.push(sendOverlapped); // Push Send Msg To User

		if (!isSending.exchange(true)) { // ���� isSending�� ��ȯ�ϸ鼭 true�� ����
			ProcSend();
		}
	}

	void SendComplete() {
		OverlappedTCP* deleteOverlapped = nullptr;

		while (deleteSendQueue.pop(deleteOverlapped)) {
			delete[] deleteOverlapped->wsaBuf.buf;
			delete deleteOverlapped;
		}

		isSending = false;
	}

private:
	void ProcSend() {
		auto sendOverlapped = new OverlappedTCP;

		if (sendQueue.pop(sendOverlapped)) {
			DWORD dwSendBytes = 0;
			int sCheck = WSASend(userSkt,
				&(sendOverlapped->wsaBuf),
				1,
				&dwSendBytes,
				0,
				(LPWSAOVERLAPPED)sendOverlapped,
				NULL);

			// -- WSASend Fail --
			if (sCheck == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				std::cout << userSkt << " WSASend Fail : " << WSAGetLastError() << std::endl;
				sendOverlapped->retryCnt++;

				if (sendOverlapped->retryCnt == MAX_RETRY_COUNT) {
					delete[] sendOverlapped->wsaBuf.buf;
					delete sendOverlapped;
					return;
				}

				sendQueue.push(sendOverlapped); // If Wsasend Fail, Try Wsasend Again
				return;
			}

			// -- Wsasend Success --
			deleteSendQueue.push(sendOverlapped);
			isSending = true;
		}

		else isSending = false; // sendQueue Empty
	}

	// 1 bytes
	bool isConn = false;
	std::atomic<bool> isSending = false;
	char acceptBuf[64];
	char recvBuf[MAX_SOCK];
	char* recvCircleBuf;

	// 2 bytes
	UINT16 connObjNum;

	// 8 bytes
	SOCKET userSkt;
	HANDLE userIocpHandle = INVALID_HANDLE_VALUE;

	// 56 bytes
	OverlappedTCP userOvlap = {};

	// 120 bytes
	CircularBuffer circularBuffer; // Make Circular Recv Buffer

	// 136 bytes 
	boost::lockfree::queue<OverlappedTCP*> sendQueue;
	boost::lockfree::queue<OverlappedTCP*> deleteSendQueue;
};