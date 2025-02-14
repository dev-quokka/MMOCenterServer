#pragma once

#include "Define.h"
#include "CircularBuffer.h"
#include "Packet.h"
#include "overLappedManager.h"

#include <cstdint>
#include <iostream>
#include <boost/lockfree/queue.hpp>

class ConnUser {
public:
	ConnUser(SOCKET UserSkt_, uint32_t bufferSize_, uint16_t connObjNum_, HANDLE sIOCPHandle, OverLappedManager* overLappedManager_) : userSkt(UserSkt_), connObjNum(connObjNum_), userIocpHandle(sIOCPHandle), overLappedManager(overLappedManager_){
		circularBuffer = std::make_unique<CircularBuffer>(bufferSize_);
	}

public :
	bool IsConn() { // check connection status
		return isConn;
	}

	char* GetRecvBuffer() {
		return recvBuf;
	}

	SOCKET GetSocket() {
		return userSkt;
	}

	uint16_t GetObjNum() {
		return connObjNum;
	}

	bool WriteRecvData(const char* data_, uint32_t size_) {
		return circularBuffer->Write(data_,size_);
	}

	PacketInfo ReadRecvData(char* readData_, uint32_t size_) { // readData_는 값을 불러오기 위한 빈 값
		if (circularBuffer->Read(readData_, size_)) {
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
		shutdown(userSkt, SD_BOTH);
		memset(acceptBuf, 0, sizeof(acceptBuf));
		memset(recvBuf, 0, sizeof(recvBuf));
		userOvlap = {};
		userIocpHandle = INVALID_HANDLE_VALUE;
		isConn = false;
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

	bool ConnUserRecv() {
		OverlappedTCP* tempOvLap = overLappedManager->getOvLap();

		if (tempOvLap == nullptr) return false;

		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		ZeroMemory(tempOvLap, sizeof(OverlappedTCP));
		tempOvLap->wsaBuf.len = MAX_SOCK;
		tempOvLap->wsaBuf.buf = recvBuf;
		tempOvLap->userSkt = userSkt;
		tempOvLap->taskType = TaskType::RECV;

		int tempR = WSARecv(userSkt,&(tempOvLap->wsaBuf),1,&dwRecvBytes, &dwFlag,(LPWSAOVERLAPPED)&(tempOvLap),NULL);
		
		if (tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << userSkt << " WSARecv() Fail : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	void PushSendMsg(const uint32_t dataSize_, char* sendMsg) {
		OverlappedTCP* overlappedTCP;
		OverlappedTCP* tempOvLap = overLappedManager->getOvLap();

		if (tempOvLap == nullptr) { // 오버랩 풀에 여분 없으면 새로 오버랩 생성
			overlappedTCP = new OverlappedTCP;
			ZeroMemory(overlappedTCP, sizeof(OverlappedTCP));
			overlappedTCP->wsaBuf.len = dataSize_;
			overlappedTCP->wsaBuf.buf = new char[dataSize_];
			CopyMemory(overlappedTCP->wsaBuf.buf, sendMsg, dataSize_);
			overlappedTCP->taskType = TaskType::SEND;

			sendQueue.push(overlappedTCP); // Push Send Msg To User
			sendQueueSize.fetch_add(1);
		}
		else {
			ZeroMemory(tempOvLap, sizeof(OverlappedTCP));
			tempOvLap->wsaBuf.len = dataSize_;
			tempOvLap->wsaBuf.buf = new char[dataSize_];
			CopyMemory(tempOvLap->wsaBuf.buf, sendMsg, dataSize_);
			tempOvLap->taskType = TaskType::SEND;

			sendQueue.push(tempOvLap); // Push Send Msg To User
			sendQueueSize.fetch_add(1);
		}

		if (sendQueueSize.load() == 1) {
			ProcSend();
		}
	}

	void SendComplete() {
		OverlappedTCP* deleteOverlapped = nullptr;

		if(sendQueue.pop(deleteOverlapped)) {
			delete[] deleteOverlapped->wsaBuf.buf;
			delete deleteOverlapped;
			sendQueueSize.fetch_sub(1);
		}

		if (sendQueueSize.load() == 1) {
			ProcSend();
		}
	}

private:
	void ProcSend() {
		OverlappedTCP* overlappedTCP;

		if (sendQueue.pop(overlappedTCP)) {
			DWORD dwSendBytes = 0;
			int sCheck = WSASend(userSkt,
				&(overlappedTCP->wsaBuf),
				1,
				&dwSendBytes,
				0,
				(LPWSAOVERLAPPED)overlappedTCP,
				NULL);
		}
	}

	// 1 bytes
	bool isConn = false;
	std::atomic<uint8_t> sendQueueSize{0};
	char acceptBuf[64] = {0};
	char recvBuf[MAX_SOCK] = {0};

	// 2 bytes
	uint16_t connObjNum;

	// 8 bytes
	SOCKET userSkt;
	HANDLE userIocpHandle = INVALID_HANDLE_VALUE;
	OverLappedManager* overLappedManager;

	// 56 bytes
	OverlappedTCP userOvlap;

	// 120 bytes
	std::unique_ptr<CircularBuffer> circularBuffer; // Make Circular Recv Buffer

	// 136 bytes 
	boost::lockfree::queue<OverlappedTCP*> sendQueue;
};





// -- WSASend Fail --

//boost::lockfree::queue<OverlappedTCP*> sendQueue;

//if (sCheck == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
//{
//	std::cout << userSkt << " WSASend Fail : " << WSAGetLastError() << std::endl;
//	sendOverlapped->retryCnt++;

//	if (sendOverlapped->retryCnt == MAX_RETRY_COUNT) {
//		delete[] sendOverlapped->wsaBuf.buf;
//		delete sendOverlapped;
//		return;
//	}

//	sendQueue.push(sendOverlapped); // If Wsasend Fail, Try Wsasend Again
//	return;
//}