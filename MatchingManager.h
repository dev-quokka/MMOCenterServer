#pragma once

#include "Room.h"
#include "Packet.h"
#include "ConnUsersManager.h"
#include "RoomManager.h"

#include <iostream>
#include <queue>
#include <set>
#include <thread>
#include <chrono>
#include <ws2tcpip.h>

#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

struct EndTimeCheck {
	uint8_t roomNum;
	std::chrono::time_point<std::chrono::steady_clock> endTime;
};

struct CompareEndTime {
	bool operator()(const EndTimeCheck& a, const EndTimeCheck& b) {
		return a.endTime < b.endTime;
	}
};

struct MatchingRoom {
	uint8_t LoofCnt = 0;
	uint8_t userLevel;
	UINT16 userSkt;
	std::string userId;
};

class MatchingManager {
public:
	void Init();
	bool Insert(uint8_t userLevel_, UINT16 userSkt_, std::string userId);
	bool CreateMatchThread();
	bool CreatTimeCheckThread();
	void MatchingThread();
	void TimeCheckThread();

private:
	// 1 bytes
	bool matchRun;
	bool timeChekcRun;

	// 16 bytes
	std::thread matchingThread;
	std::thread timeCheckThread;

	// 24 bytes
	std::multiset<EndTimeCheck, CompareEndTime> roomExpiration;

	// 40 bytes
	std::priority_queue<EndTimeCheck, std::vector<EndTimeCheck>, std::greater<>> rtCheckQueue;

	// 136 bytes
	boost::lockfree::queue<uint8_t> roomNumQueue; // Set Room Num

	// 576 bytes
	tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>> matchingMap; // {Level/3 + 1 (0~2 = 1, 3~5 = 2 ...), UserSkt}

	// 606 bytes
	ConnUsersManager* connUsersManager;
};