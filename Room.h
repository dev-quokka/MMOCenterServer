#pragma 
#include <iostream>
#include <ws2tcpip.h>

class Room {
public:
	Room(uint8_t roomNum_, UINT16 userSkt1_, UINT16 userSkt2_, int mobHp_) : roomNum(roomNum_), userSkt1(userSkt1_), userSkt2(userSkt2_), mobHp(mobHp_) {}
	void MakeRoom();
	void Hit();

private:
	uint8_t roomNum;
	UINT16 userSkt1;
	UINT16 userSkt2;
	int mobHp;
};