#include "UdpOverLappedManager.h"

void UdpOverLappedManager::init() {
	for (int i = 0; i < OVERLAPPED_UDP_QUEUE_SIZE; i++) {
		OverlappedUDP* overlappedUDP = new OverlappedUDP; // ����
		ZeroMemory(overlappedUDP, sizeof(OverlappedUDP)); // �ʱ�ȭ
		ovLapPool.push(overlappedUDP);
	}
}

OverlappedUDP* UdpOverLappedManager::getOvLap() {
	OverlappedUDP* overlappedUDP_;
	if (ovLapPool.pop(overlappedUDP_)) {
		return overlappedUDP_;
	}
	else return nullptr;
}

void UdpOverLappedManager::returnOvLap(OverlappedUDP* overlappedUDP_) {
	delete[] overlappedUDP_->wsaBuf.buf;
	ZeroMemory(overlappedUDP_, sizeof(OverlappedUDP)); // �ʱ�ȭ
	ovLapPool.push(overlappedUDP_);
}