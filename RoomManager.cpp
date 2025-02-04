#include "RoomManager.h"

void RoomManager::MakeRoom(uint8_t roomNum_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_, std::chrono::time_point<std::chrono::steady_clock> endTime_) {
	Room* room;
	room->set(roomNum_, mobHp_, user1_,user2_, endTime_);
	roomMap[roomNum_] = room;
	//roomMap.emplace(roomNum_, room);
}

void RoomManager::DeleteRoom(uint8_t roomNum_) {
	Room* room = roomMap[roomNum_];
	delete room;
	roomMap.erase(roomNum_);
}