#pragma once

#include <set>
#include <thread>
#include <chrono>
#include <queue>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <boost/lockfree/queue.hpp>
#include <tbb/concurrent_hash_map.h>

#include "Define.h"
#include "RoomManager.h"
#include "InGameUserManager.h"
#include "RedisManager.h"

constexpr int UDP_PORT = 50000;
constexpr uint8_t USER_MAX_LEVEL = 15;

class Room;
class RoomManager;
class ConnUsersManager;
class RedisManager;

struct EndTimeComp {
	bool operator()(Room* r1, Room* r2) const {
		return r1->GetEndTime() < r2->GetEndTime();
	}
};

struct MatchingRoom {
	uint8_t LoofCnt = 0;
	uint8_t userLevel;
	SOCKET userSkt;
	std::string userId;
};

class MatchingManager {
public:
	~MatchingManager() {
		matchRun = false;
		timeChekcRun = false;
		workRun = false;

		if (matchingThread.joinable()) {
			matchingThread.join();
		}

		if (timeCheckThread.joinable()) {
			timeCheckThread.join();
		}

		if (udpWorkThread.joinable()) {
			udpWorkThread.join();
		}

		for (int i = 0; i < USER_MAX_LEVEL; i++) {
			tbb::concurrent_hash_map<uint8_t, std::priority_queue<MatchingRoom*>>::accessor accessor;

			if (matchingMap.find(accessor, i)) {
				std::priority_queue<MatchingRoom*> temp = accessor->second;
				while (!temp.empty()) {
					delete temp.top();
					temp.pop();
				}
			}

		}
	}

	void Init(const uint16_t maxClientCount_, RedisManager* redisManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_);
	bool Insert(uint8_t userLevel_, SOCKET userSkt_, std::string userId);
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
	SOCKET udpSocket; // 1 Socket Of 300 Users 

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

	// 776 bytes
	RedisManager* redisManager;
};