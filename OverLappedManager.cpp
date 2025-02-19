#include "OverLappedManager.h"

void OverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_TCP_QUEUE_SIZE; i++) {
		OverlappedTCP* overlappedTCP = new OverlappedTCP; // ����
		ZeroMemory(overlappedTCP, sizeof(OverlappedTCP)); // �ʱ�ȭ
		ovLapPool.push(overlappedTCP);
	}
}

OverlappedTCP* OverLappedManager::getOvLap() {
	std::cout << " ������ ������ " << std::endl;
	OverlappedTCP* overlappedTCP_;
	if (ovLapPool.pop(overlappedTCP_)) {
		return overlappedTCP_;
	}
	else return nullptr;
}

void OverLappedManager::returnOvLap(OverlappedTCP* overlappedTCP_){
	delete[] overlappedTCP_->wsaBuf.buf;
	ZeroMemory(overlappedTCP_, sizeof(OverlappedTCP)); // �ʱ�ȭ
	ovLapPool.push(overlappedTCP_);
}