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
	void set(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, UINT16 userSkt1_, UINT16 userSkt2_, InGameUser* user1_, InGameUser* user2_) {
		RaidUserInfo ruInfo1(userSkt1_, user1_);
		ruInfos.emplace_back(ruInfo1);
		RaidUserInfo ruInfo2(userSkt2_, user2_);
		ruInfos.emplace_back(ruInfo2);
		mobHp = mobHp_;
	}

	bool StartCheck() {
		if (startCheck.fetch_add(1) + 1 == 2) {
			endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2)+ std::chrono::seconds(8);
		}
	}

	uint8_t GetRoomNum() {
		return roomNum;
	}

	InGameUser* GetUser(uint8_t userNum_) {
		if (userNum_ == 0) return ruInfos[0].inGameUser;
		else if (userNum_ == 1) return ruInfos[1].inGameUser;
	}

	InGameUser* GetTeamUser(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].inGameUser;
		else if (userNum_ == 0) return ruInfos[1].inGameUser;
	}

	UINT16 GetTeamSkt(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].userSkt;
		else if (userNum_ == 0) return ruInfos[1].userSkt;
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

	unsigned int Hit(uint8_t userNum, unsigned int damage_){ // userNum으로 하면 해킹 위험 있을 수도 있으니 받은 소켓으로 확인
		unsigned int hitMob = mobHp.fetch_sub(damage_);

		if (mobHp <= 0 || finishCheck.load()) {
			return 0;
		}

		unsigned int myDamage = min(damage_,hitMob);

		if (myDamage <= 0) {
			finishCheck.store(true);
			myDamage = mobHp + damage_;
		}

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
				int k;
				if ((k = mobHp.load() - damage_) <= 0) {
					mobHp.store(0);
					ruInfos[1].userScore = k + damage_;

					MatchingManager* matchingManager;
					matchingManager->DeleteMob(this); // delete room 요청

					return 3;
				}
				else {
					mobHp.fetch_sub(damage_);
					ruInfos[1].userScore = damage_;
					return 1;
				}
			}
	}

private:
	// 1 bytes
	uint8_t roomNum;
	std::atomic<bool> finishCheck = false;
	std::atomic<uint8_t> startCheck = 0;

	// 4 bytes
	std::atomic<unsigned int> mobHp;

	std::vector<RaidUserInfo> ruInfos;

	// 8 bytes
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now()+ std::chrono::minutes(2); // 생성 되자마자 삭제 방지

};