#include "OverLappedManager.h"

void OverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_TCP_QUEUE_SIZE; i++) {
		OverlappedTCP* overlappedTCP = new OverlappedTCP;
		ZeroMemory(overlappedTCP, sizeof(overlappedTCP));
		ovLapPool.push(overlappedTCP);
	}
}

OverlappedTCP* OverLappedManager::getOvLap() {
	OverlappedTCP* overlappedTCP_;
	if (ovLapPool.pop(overlappedTCP_)) {
		return overlappedTCP_;
	}
	else return nullptr;
}

void OverLappedManager::returnOvLap(OverlappedTCP* overlappedTCP_){
	ovLapPool.push(overlappedTCP_);
}