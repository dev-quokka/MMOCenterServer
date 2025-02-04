#pragma 
#include <iostream>
#include <chrono>
#include <ws2tcpip.h>

class Room {
public:
	bool set(uint8_t roomNum_, unsigned int mobHp_, InGameUser* user1_, InGameUser* user2_, std::chrono::time_point<std::chrono::steady_clock> endTime_) {
	
	}

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		return endTime;
	}

	uint8_t Hit(unsigned int damage_){ // Success(1), Fail(2), Zero(3)
		if (mobHp.load() <= 0) {
			return 3;
		}
		else {
			if (mobHp.load() - damage_ <= 0) {
				mobHp.store(0);
				return 3;
			}
			else {
				mobHp.fetch_sub(damage_);
				return 1;
			}
		}
	}
	
	std::pair<unsigned int, unsigned int> getScore() {
		return { user1Score ,user2Score };
	}

private:
	// 1 bytes
	uint8_t roomNum;

	// 4 bytes
	std::atomic<unsigned int> mobHp;
	std::atomic<unsigned int> user1Score = 0;
	std::atomic<unsigned int> user2Score = 0;

	// 8 bytes
	std::chrono::time_point<std::chrono::steady_clock> endTime;

	InGameUser* user1; 
	InGameUser* user2;
};