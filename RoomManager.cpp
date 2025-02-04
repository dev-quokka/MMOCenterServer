#include "RoomManager.h"

void RoomManager::MakeRoom(uint8_t roomNum, Room* room) {
	roomMap.emplace(roomNum, room);
}