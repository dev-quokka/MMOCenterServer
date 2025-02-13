#pragma once
#include "Room.h"
#include "InGameUser.h"
#include <chrono>
#include <unordered_map>

//#include <tbb/concurrent_hash_map.h>

class RoomManager {
public:
	Room* MakeRoom(MatchingManager* matchingManager_, uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, UINT16 userSkt1_, UINT16 userSkt2_, InGameUser* user1_, InGameUser* user2_);
	Room* GetRoom(uint8_t roomNum_);
	bool DeleteRoom(uint8_t roomNum);

private:
	std::unordered_map<uint8_t, Room*> roomMap; // { roomNum, Room }

	//// 576 bytes
	//tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}
};