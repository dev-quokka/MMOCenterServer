#include "OverLappedManager.h"

void OverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_TCP_QUEUE_SIZE; i++) {
		OverlappedTCP* overlappedTCP = new OverlappedTCP;
		ZeroMemory(overlappedTCP, sizeof(overlappedTCP));
		overlappedTCP->wsaBuf.len = MAX_RECV_SIZE;
		overlappedTCP->wsaBuf.buf = new char[MAX_RECV_SIZE]; // 내 구조에서 송 수신 최대값을 미리 세팅해두기
		ovLapPool.push(overlappedTCP);
	}
}

OverlappedTCP* OverLappedManager::getOvLap() {
	std::cout << " 오버랩 가져감 " << std::endl;
	OverlappedTCP* overlappedTCP_;
	if (ovLapPool.pop(overlappedTCP_)) {
		return overlappedTCP_;
	}
	else return nullptr;
}

void OverLappedManager::returnOvLap(OverlappedTCP* overlappedTCP_){
	overlappedTCP_->userSkt = 0;
	memset(overlappedTCP_->wsaBuf.buf, 0, sizeof(overlappedTCP_->wsaBuf.buf));
	ZeroMemory(&(overlappedTCP_->wsaOverlapped), sizeof(overlappedTCP_->wsaOverlapped));
	ovLapPool.push(overlappedTCP_);
}