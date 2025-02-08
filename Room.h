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
			return true;
		}
		return false;
	}

	bool EndCheck() {
		if (startCheck.fetch_sub(1) + 1 == 0) {
			return true;
		}
		return false;
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

	unsigned int Hit(uint8_t userNum_, unsigned int damage_){
		if (mobHp <= 0 || finishCheck.load()) {
			return 0;
		}

		if (mobHp.fetch_sub(damage_)-damage_<=0) {
			finishCheck.store(true);
			return ruInfos[userNum_].userScore.fetch_add(mobHp + damage_) + (mobHp + damage_);

			MatchingManager* matchingManager;
			matchingManager->DeleteMob(this); // delete room 요청
		}

		return ruInfos[userNum_].userScore.fetch_add(damage_) + damage_;
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