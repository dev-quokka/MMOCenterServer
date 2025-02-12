#pragma once

#include "RoomManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

#include <iostream>
#include <queue>
#include <set>
#include <thread>
#include <chrono>
#include <ws2tcpip.h>

#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

constexpr int UDP_PORT = 50000;

struct EndTimeComp {
	bool operator()(Room* r1, Room* r2) const {
		return r1->GetEndTime() < r2->GetEndTime();
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
	void Init(const UINT16 maxClientCount_, RedisManager* redisManager);
	bool Insert(uint8_t userLevel_, UINT16 userSkt_, std::string userId);
	bool CreateMatchThread();
	bool CreateUDPWorkThread();
	bool CreateTimeCheckThread();
	void UDPWorkThread();
	void MatchingThread();
	void TimeCheckThread();
	void DeleteMob(Room* room_);
	void SyncMobHp(OverlappedUDP* overlappedUDP_);
	SOCKET GetUDPSocket();

private:
	// 1 bytes
	bool matchRun;
	bool timeChekcRun;
	bool workRun;

	// 8 bytes
	HANDLE udpHandle;
	SOCKET udpSocket;

	// 16 bytes
	std::thread udpWorkThread;
	std::thread matchingThread;
	std::thread timeCheckThread;
	char serverIP[16];

	// 24 bytes
	std::set<Room*, EndTimeComp> endRoomCheckSet;

	// 64 bytes
	InGameUserManager* inGameUserManager;

	// 80 bytes
	std::mutex mDeleteRoom;

	// 136 bytes
	boost::lockfree::queue<uint8_t> roomNumQueue; // Set RoomNum

	// 576 bytes
	RoomManager* roomManager;
	tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>> matchingMap; // {Level/3 + 1 (0~2 = 1, 3~5 = 2 ...), UserSkt}

	// 606 bytes
	ConnUsersManager* connUsersManager;

	// 760 bytes
	RedisManager* redisManager;
};