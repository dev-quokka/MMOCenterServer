#pragma once
#include "Room.h"
#include <tbb/concurrent_hash_map.h>

class RoomManager {
public:
	void MakeRoom(uint8_t roomNum, Room* room);
	

private:
	// 576 bytes
	tbb::concurrent_hash_map<uint8_t, Room*> roomMap; // {roomNum, Room}
};