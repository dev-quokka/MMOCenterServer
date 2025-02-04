#pragma 
#include <iostream>
#include <ws2tcpip.h>

class Room {
public:
	Room(uint8_t roomNum_, UINT16 userSkt1_, UINT16 userSkt2_, int mobHp_) : roomNum(roomNum_), userSkt1(userSkt1_), userSkt2(userSkt2_), mobHp(mobHp_) {}
	void Hit();

private:
	// 1 bytes
	uint8_t roomNum;
	uint8_t timer; // minutes
	uint8_t userLevel1;
	uint8_t userLevel2; // minutes

	// 2 bytes
	UINT16 userSkt1;
	UINT16 userSkt2;

	// 4 bytes
	int mobHp;

	// 40 bytes
	std::string userId1;
	std::string userId2;
};