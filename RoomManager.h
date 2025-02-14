#pragma once
#include <chrono>
#include <cstdint>
#include <unordered_map>

#include "Room.h"

class InGameUser;

class RoomManager {
public:
	RoomManager(SOCKET* udpSkt_) : udpSkt(udpSkt_) {}
	~RoomManager() {
		for (auto& iter : roomMap) {
			delete iter.second;
		}
	}

	bool DeleteRoom(uint8_t roomNum);
	Room* MakeRoom(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, uint16_t userSkt1_, uint16_t userSkt2_, InGameUser* user1_, InGameUser* user2_);
	Room* GetRoom(uint8_t roomNum_);

private:
	SOCKET* udpSkt;
	std::unordered_map<uint8_t, Room*> roomMap; // { roomNum, Room }
};



// #include <tbb/concurrent_hash_map.h>
// 576 bytes
// tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}