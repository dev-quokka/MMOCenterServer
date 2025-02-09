#pragma 
#include "MatchingManager.h"

#include <iostream>
#include <chrono>
#include <ws2tcpip.h>

struct RaidUserInfo {
	std::atomic<unsigned int> userScore = 0;
	UINT16 userSkt; // TCP Socket
	sockaddr_in userAddr;
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

	void setSockAddr(uint8_t userNum_, sockaddr_in userAddr_) {
		ruInfos[userNum_].userAddr = userAddr_;
	}

	bool StartCheck() {
		if (startCheck.fetch_add(1) + 1 == 2) {
			endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2)+ std::chrono::seconds(8);
			matchingManager = std::make_unique<MatchingManager>();
			udpSkt = matchingManager->GetUDPSocket(roomNum);
			overlappedUDP = new OverlappedUDP;
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

	UINT16 GetTeamSkt(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].userSkt;
		else if (userNum_ == 0) return ruInfos[1].userSkt;
	}

	InGameUser* GetTeamUser(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].inGameUser;
		else if (userNum_ == 0) return ruInfos[1].inGameUser;
	}

	unsigned int Hit(uint8_t userNum_, unsigned int damage_){
		if (mobHp <= 0 || finishCheck.load()) {
			return 0;
		}
		
		unsigned int score_;
		unsigned int currentMobHp_;

		if (mobHp.fetch_sub(damage_)-damage_<=0) {
			finishCheck.store(true);
			score_ = ruInfos[userNum_].userScore.fetch_add(mobHp + damage_) + (mobHp + damage_);
			return score_;
			matchingManager->DeleteMob(this); // Delete Room Request
		}

		for (int i = 0; i < ruInfos.size(); i++) {
			ZeroMemory(overlappedUDP, sizeof(OverlappedUDP));
			overlappedUDP->wsaBuf.len = sizeof(currentMobHp_);
			overlappedUDP->wsaBuf.buf = new char[sizeof(currentMobHp_)];
			CopyMemory(overlappedUDP->wsaBuf.buf, &currentMobHp_, sizeof(currentMobHp_));
			sockaddr_in userAddr = ruInfos[i].userAddr;
			overlappedUDP->addrSize = sizeof(userAddr);
			overlappedUDP->userAddr = userAddr;
			overlappedUDP->taskType = TaskType::SEND;

			matchingManager->SyncMobHp(overlappedUDP,roomNum); // Syncronize Mob Hp
		}

		score_ = ruInfos[userNum_].userScore.fetch_add(damage_) + damage_;
		return score_;
	}

private:
	// 1 bytes
	uint8_t roomNum;
	std::atomic<bool> finishCheck = false;
	std::atomic<uint8_t> startCheck = 0;

	// 4 bytes
	std::atomic<unsigned int> mobHp;

	// 8 bytes
	SOCKET udpSkt;
	OverlappedUDP* overlappedUDP;
	std::unique_ptr<MatchingManager> matchingManager;
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now()+ std::chrono::minutes(2); // 생성 되자마자 삭제 방지

	// 32 bytes
	std::vector<RaidUserInfo> ruInfos;

	// 256 bytes
	char buffer[64];
};