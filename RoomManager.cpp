#include "RoomManager.h"

Room* RoomManager::MakeRoom(uint8_t roomNum_, unsigned int mobHp_, UINT16 userSkt1_, UINT16 userSkt2_, InGameUser* user1_, InGameUser* user2_, std::chrono::time_point<std::chrono::steady_clock> endTime_) {
	Room* room;
	room->set(roomNum_, mobHp_, userSkt1_, userSkt2_, user1_,user2_, endTime_);
	roomMap[roomNum_] = room;
	return room;
	//roomMap.emplace(roomNum_, room); => ���߿� ��Ī ������ �þ�� concurrent_hash_map ���
}

bool RoomManager::DeleteRoom(uint8_t roomNum_) {
	Room* room = roomMap[roomNum_];
	delete room;
	roomMap.erase(roomNum_);
	return true;
}