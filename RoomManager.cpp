#include "RoomManager.h"

Room* RoomManager::MakeRoom(uint16_t roomNum_, uint16_t userObjNum1_, uint16_t userObjNum2_, InGameUser* user1_, InGameUser* user2_) {
	Room* room = new Room(udpSkt);
	room->set(roomNum_, userObjNum1_, userObjNum2_, user1_,user2_);
	roomMap[roomNum_] = room;
	return room;
	//roomMap.emplace(roomNum_, room); => ���߿� ��Ī ������ �þ�� concurrent_hash_map ���
}

Room* RoomManager::GetRoom(uint16_t roomNum_) {
	return roomMap[roomNum_];
}

bool RoomManager::DeleteRoom(uint16_t roomNum_) {
	Room* room = roomMap[roomNum_];
	delete room;
	roomMap.erase(roomNum_);
	return true;
}