#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>

#include "Room.h"
#include "MatchingManager.h"
#include "InGameUser.h"

class RoomManager {
public:
	RoomManager(MatchingManager* matchingManager_) : matchingManager(matchingManager_) {}

	~RoomManager() {
		for (auto& iter : roomMap) {
			delete iter.second;
		}
	}

	bool DeleteRoom(uint8_t roomNum);
	Room* MakeRoom(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, uint16_t userSkt1_, uint16_t userSkt2_, InGameUser* user1_, InGameUser* user2_);
	Room* GetRoom(uint8_t roomNum_);

private:
	std::unordered_map<uint8_t, Room*> roomMap; // { roomNum, Room }
	MatchingManager* matchingManager;
};



// #include <tbb/concurrent_hash_map.h>
// 576 bytes
// tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}