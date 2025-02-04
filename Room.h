#pragma 
#include <iostream>
#include <ws2tcpip.h>

class Room {
public:
	bool set(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_) {
	
	}

	uint8_t Hit(unsigned int damage_) { // Success(1), Fail(2), Zero(3)
		if (mobHp.load() <= 0) {
			return 3;
		}
		else {
			mobHp.fetch_sub(damage_);
			return 1;
		}
	}

	void SetScore() {

	}

private:
	// 1 bytes
	uint8_t roomNum;
	uint8_t timer; // minutes

	// 4 bytes
	std::atomic<unsigned int> mobHp;

	InGameUser* user1; 
	InGameUser* user2;
};