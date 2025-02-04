#pragma once
#include "Room.h"
#include "InGameUser.h"
#include <tbb/concurrent_hash_map.h>

class RoomManager {
public:
	void MakeRoom(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_);
	void DeleteRoom(uint8_t roomNum);

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}
};