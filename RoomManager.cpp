#include "RoomManager.h"

void RoomManager::MakeRoom(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_) {
	Room* room(roomNum_,timer_, mobHp_, user1_,user2_);
	roomMap.emplace(roomNum_, &room);
}

void DeleteRoom(uint8_t roomNum) {
	
}