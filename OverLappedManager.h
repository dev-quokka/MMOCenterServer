#pragma once

#include "Define.h"
#include <iostream>
#include <boost/lockfree/queue.hpp>

constexpr uint16_t OVERLAPPED_TCP_QUEUE_SIZE = 50;

class OverLappedManager {
public:
	~OverLappedManager() {
		std::cout << "오버랩매니저 삭제 시작" << std::endl;
		OverlappedTCP* overlappedTCP;
		while (ovLapPool.pop(overlappedTCP)) {	
			delete overlappedTCP->wsaBuf.buf;
			delete overlappedTCP;
		}
		std::cout << "오버랩 매니저 삭제" << std::endl;
	}

	void init();
	OverlappedTCP* getOvLap();
	void returnOvLap(OverlappedTCP* overlappedTCP_);
private:
	boost::lockfree::queue<OverlappedTCP*> ovLapPool{ OVERLAPPED_TCP_QUEUE_SIZE };
};