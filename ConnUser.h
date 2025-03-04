#pragma once

#include "Define.h"
#include "CircularBuffer.h"
#include "Packet.h"
#include "overLappedManager.h"

#include <cstdint>
#include <iostream>
#include <atomic>
#include <boost/lockfree/queue.hpp>

class ConnUser {
public:

	ConnUser(uint32_t bufferSize_, uint16_t connObjNum_, HANDLE sIOCPHandle_, OverLappedManager* overLappedManager_) 
		: connObjNum(connObjNum_), sIOCPHandle(sIOCPHandle_), overLappedManager(overLappedManager_){
		userSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (userSkt == INVALID_SOCKET) {
			std::cout << "Client socket Error : " << GetLastError() << std::endl;
		}

		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)userSkt, sIOCPHandle_, (ULONG_PTR)0, 0);

		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort()�Լ� ���� :" << GetLastError() << std::endl;
		}

		circularBuffer = std::make_unique<CircularBuffer>(bufferSize_);
	}
	~ConnUser() {
		struct linger stLinger = { 0, 0 };	// SO_DONTLINGER�� ����

		stLinger.l_onoff = 0;

		shutdown(userSkt, SD_BOTH);

		setsockopt(userSkt, SOL_SOCKET, SO_LINGER, (char*)&stLinger, sizeof(stLinger));

		closesocket(userSkt);
	}

public :
	bool IsConn() { // check connection status
		return isConn;
	}

	SOCKET GetSocket() {
		return userSkt;
	}

	uint16_t GetObjNum() {
		return connObjNum;
	}

	void SetPk(uint32_t userPk_) {
		userPk = userPk_;
	}

	uint32_t GetPk() {
		return userPk;
	}

	bool WriteRecvData(const char* data_, uint32_t size_) {
		return circularBuffer->Write(data_,size_);
	}

	PacketInfo ReadRecvData(char* readData_, uint32_t size_) { // readData_�� ���� �ҷ����� ���� �� ��
		CopyMemory(readData, readData_, size_);

		if (circularBuffer->Read(readData, size_)) {
			auto pHeader = (PACKET_HEADER*)readData;

			PacketInfo packetInfo;
			packetInfo.packetId = pHeader->PacketId;
			packetInfo.dataSize = pHeader->PacketLength;
			packetInfo.connObjNum = connObjNum;
			packetInfo.pData = readData;

			return packetInfo;
		}
	}

	void Reset() {
		isConn = false;
		shutdown(userSkt, SD_BOTH);
		closesocket(userSkt);
		//memset(acceptBuf, 0, sizeof(acceptBuf));
		//acceptOvlap = {};
		userSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (userSkt == INVALID_SOCKET) {
			std::cout << "Client socket Error : " << GetLastError() << std::endl;
		}

		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)userSkt, sIOCPHandle, (ULONG_PTR)0, 0);

		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort()�Լ� ���� :" << GetLastError() << std::endl;
		}

	}

	bool PostAccept(SOCKET ServerSkt_) {
		acceptOvlap = {};

		acceptOvlap.taskType = TaskType::ACCEPT;
		acceptOvlap.connObjNum = connObjNum;
		acceptOvlap.wsaBuf.buf = nullptr;
		acceptOvlap.wsaBuf.len = 0;

		DWORD bytes = 0;
		DWORD flags = 0;

		if (AcceptEx(ServerSkt_, userSkt, acceptBuf,0,sizeof(SOCKADDR_IN)+16, sizeof(SOCKADDR_IN) + 16,&bytes,(LPWSAOVERLAPPED)&acceptOvlap)==0) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				std::cout << "AcceptEx Error : " << GetLastError() << std::endl;
				return false;
			}
		}

		return true;
	}

	bool ConnUserRecv() {

		OverlappedTCP* tempOvLap = (overLappedManager->getOvLap());

		if (tempOvLap == nullptr) return false;

		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
		tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
		tempOvLap->connObjNum = connObjNum;
		tempOvLap->taskType = TaskType::RECV;

		int tempR = WSARecv(userSkt,&(tempOvLap->wsaBuf),1,&dwRecvBytes, &dwFlag,(LPWSAOVERLAPPED)tempOvLap,NULL);

		if(tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << userSkt << " WSARecv() Fail : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	void PushSendMsg(const uint32_t dataSize_, char* sendMsg) {

		OverlappedTCP* tempOvLap = overLappedManager->getOvLap();

		if (tempOvLap == nullptr) { // ������ Ǯ�� ���� ������ ���� ������ ����
			OverlappedTCP* overlappedTCP = new OverlappedTCP;
			ZeroMemory(overlappedTCP, sizeof(OverlappedTCP));
			overlappedTCP->wsaBuf.len = MAX_RECV_SIZE;
			overlappedTCP->wsaBuf.buf = new char[MAX_RECV_SIZE];
			overlappedTCP->connObjNum = connObjNum;
			CopyMemory(overlappedTCP->wsaBuf.buf, sendMsg, dataSize_);
			overlappedTCP->taskType = TaskType::NEWSEND;

			sendQueue.push(overlappedTCP); // Push Send Msg To User
			sendQueueSize.fetch_add(1);
		}
		else {
			tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
			tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
			tempOvLap->connObjNum = connObjNum;
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
		sendQueueSize.fetch_sub(1);

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
	std::atomic<uint16_t> sendQueueSize{ 0 };

	// 2 bytes
	uint16_t connObjNum;

	// 4 bytes
	uint32_t userPk;

	// 8 bytes
	SOCKET userSkt;
	HANDLE sIOCPHandle;
	OverLappedManager* overLappedManager;

	// 56 bytes
	OverlappedTCP acceptOvlap;

	// 64 bytes
	char acceptBuf[64] = { 0 };

	// 120 bytes
	std::unique_ptr<CircularBuffer> circularBuffer;

	// 136 bytes 
	boost::lockfree::queue<OverlappedTCP*> sendQueue{10};

	char recvBuf[1024] = { 0 };
	char readData[1024] = {0};
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