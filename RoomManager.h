#pragma once
#include "Room.h"
#include "InGameUser.h"
#include <chrono>
#include <unordered_map>

//#include <tbb/concurrent_hash_map.h>

class RoomManager {
public:
	void MakeRoom(uint8_t roomNum_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_, std::chrono::time_point<std::chrono::steady_clock> endTime_);
	void DeleteRoom(uint8_t roomNum);

private:
	std::unordered_map<uint8_t, Room*> roomMap; // { roomNum, Room }

	//// 576 bytes
	//tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}
};