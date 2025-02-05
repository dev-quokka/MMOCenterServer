#pragma 
#include "MatchingManager.h"

#include <iostream>
#include <chrono>
#include <ws2tcpip.h>

struct RaidUserInfo {
	std::atomic<unsigned int> userScore = 0;
	UINT16 userSkt;
	InGameUser* inGameUser;
	RaidUserInfo(UINT16 userSkt_, InGameUser* inGameUser_) : userSkt(userSkt_), inGameUser(inGameUser_) {}
};

class Room {
public:
	void set(uint8_t roomNum_, unsigned int mobHp_, UINT16 userSkt1_, UINT16 userSkt2_, InGameUser* user1_, InGameUser* user2_, std::chrono::time_point<std::chrono::steady_clock> endTime_) {
		RaidUserInfo ruInfo1(userSkt1_, user1_);
		ruInfos.emplace_back(ruInfo1);
		RaidUserInfo ruInfo2(userSkt2_, user2_);
		ruInfos.emplace_back(ruInfo2);
	}

	uint8_t GetRoomNum() {
		return roomNum;
	}

	InGameUser* GetUser(uint8_t userNum) {
		if (userNum == 0) return ruInfos[0].inGameUser;
		else if (userNum == 1) return ruInfos[1].inGameUser;
	}

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		return endTime;
	}

	UINT16 GetUserSkt(uint8_t userNum) {
		if (userNum == 0) return ruInfos[0].userSkt;
		else if (userNum == 1) return ruInfos[1].userSkt;
	}

	unsigned int GetScore(uint8_t userNum) {
		if (userNum == 0) return ruInfos[0].userScore;
		else if (userNum == 1) return ruInfos[1].userScore;
	}

	uint8_t Hit(UINT16 userSkt_, unsigned int damage_){ // Success(1), Fail(2), Zero(3)
		if (finishCheck) { // If Times Up, Zero Damage
			return 0;
		}
		if (mobHp.load() <= 0) {
			return 3;
		}
		else {
			if (ruInfos[0].userSkt == userSkt_) {
				int k;
				if ((k = mobHp.load() - damage_) <= 0) {
					mobHp.store(0);
					ruInfos[0].userScore = k + damage_;

					MatchingManager* matchingManager;
					matchingManager->DeleteMob(this); // delete room 요청

					return 3;
				}
				else {
					mobHp.fetch_sub(damage_);
					ruInfos[0].userScore = damage_;
					return 1;
				}
			}
			else {
				if (mobHp.load() - damage_ <= 0) {
					mobHp.store(0);
					return 3;
				}
				else {
					mobHp.fetch_sub(damage_);
					ruInfos[1].userScore = damage_;
					// 레이드 종료신호 보내기

					return 1;
				}
			}
		}
	}

private:
	// 1 bytes
	uint8_t roomNum;
	std::atomic<bool> finishCheck = false;

	// 4 bytes
	std::atomic<unsigned int> mobHp;

	std::vector<RaidUserInfo> ruInfos;

	// 8 bytes
	std::chrono::time_point<std::chrono::steady_clock> endTime;

};