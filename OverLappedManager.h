#pragma once

#include "Define.h"
#include <iostream>
#include <boost/lockfree/queue.hpp>

constexpr uint16_t OVERLAPPED_TCP_QUEUE_SIZE = 50;

class OverLappedManager {
public:
	~OverLappedManager() {
		std::cout << "�������Ŵ��� ���� ����" << std::endl;
		OverlappedTCP* overlappedTCP;
		while (ovLapPool.pop(overlappedTCP)) {	
			delete overlappedTCP->wsaBuf.buf;
			delete overlappedTCP;
		}
		std::cout << "������ �Ŵ��� ����" << std::endl;
	}

	void init();
	OverlappedTCP* getOvLap();
	void returnOvLap(OverlappedTCP* overlappedTCP_);
private:
	boost::lockfree::queue<OverlappedTCP*> ovLapPool{ OVERLAPPED_TCP_QUEUE_SIZE };
};