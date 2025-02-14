#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Define.h"

class MatchingManager;
class InGameUser;

struct RaidUserInfo {
	std::atomic<unsigned int> userScore = 0;
	uint16_t userSkt; // TCP Socket
	sockaddr_in userAddr;
	InGameUser* inGameUser;
	OverlappedUDP* hpOverlapped = new OverlappedUDP;
	RaidUserInfo(uint16_t userSkt_, InGameUser* inGameUser_) : userSkt(userSkt_), inGameUser(inGameUser_) {}
};

class Room {
public:
	Room(SOCKET* udpSkt_) : udpSkt(udpSkt_){}

	~Room() {
		for (int i = 0; i < ruInfos.size(); i++) {
			delete[] ruInfos[i].hpOverlapped->wsaBuf.buf;
			delete ruInfos[i].hpOverlapped;
		}
	}

	void set(uint8_t roomNum_, uint8_t timer_, unsigned int mobHp_, uint16_t userSkt1_, uint16_t userSkt2_, InGameUser* user1_, InGameUser* user2_) {
		RaidUserInfo ruInfo1(userSkt1_, user1_);
		ruInfos.emplace_back(ruInfo1);
		RaidUserInfo ruInfo2(userSkt2_, user2_);
		ruInfos.emplace_back(ruInfo2);
		mobHp.store(mobHp_);
	}

	void setSockAddr(uint8_t userNum_, sockaddr_in userAddr_) {
		ruInfos[userNum_].userAddr = userAddr_;
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

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		return endTime;
	}

	uint16_t GetUserSkt(uint8_t userNum) {
		if (userNum == 0) return ruInfos[0].userSkt;
		else if (userNum == 1) return ruInfos[1].userSkt;
	}

	unsigned int GetScore(uint8_t userNum) {
		if (userNum == 0) return ruInfos[0].userScore;
		else if (userNum == 1) return ruInfos[1].userScore;
	}

	uint16_t GetTeamSkt(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].userSkt;
		else if (userNum_ == 0) return ruInfos[1].userSkt;
	}

	InGameUser* GetTeamUser(uint8_t userNum_) {
		if (userNum_ == 1) return ruInfos[0].inGameUser;
		else if (userNum_ == 0) return ruInfos[1].inGameUser;
	}

	std::pair<unsigned int, unsigned int> Hit(uint8_t userNum_, unsigned int damage_){ // current mob hp, score
		if (mobHp <= 0 || finishCheck.load()) {
			return {0,0};
		}
		
		unsigned int score_;
		unsigned int currentMobHp_;

		if ((currentMobHp_ = mobHp.fetch_sub(damage_))-damage_<=0) { // Hit
			finishCheck.store(true);
			score_ = ruInfos[userNum_].userScore.fetch_add(mobHp + damage_) + (mobHp + damage_);
			return { 0,score_ };
		}

		score_ = ruInfos[userNum_].userScore.fetch_add(damage_) + damage_;
		
		for (int i = 0; i < ruInfos.size(); i++) { // 나머지 유저들에게도 바뀐 몹 hp값 보내주기
			OverlappedUDP* overlappedUDP = ruInfos[i].hpOverlapped;
			ZeroMemory(overlappedUDP, sizeof(OverlappedUDP));
			overlappedUDP->wsaBuf.len = sizeof(currentMobHp_);
			overlappedUDP->wsaBuf.buf = new char[sizeof(currentMobHp_)];
			CopyMemory(overlappedUDP->wsaBuf.buf, &currentMobHp_, sizeof(currentMobHp_));
			overlappedUDP->addrSize = sizeof(ruInfos[i].userAddr);
			overlappedUDP->userAddr = ruInfos[i].userAddr;
			overlappedUDP->taskType = TaskType::SEND;

			DWORD dwSendBytes = 0;
			int result = WSASendTo(*udpSkt, &overlappedUDP->wsaBuf, 1, &dwSendBytes, 0, (SOCKADDR*)&overlappedUDP->userAddr, sizeof(overlappedUDP->userAddr), (LPWSAOVERLAPPED)overlappedUDP, NULL);

			if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
				std::cerr << "WSASendTo Fail : " << WSAGetLastError() << std::endl;
				delete[] overlappedUDP->wsaBuf.buf;
				delete overlappedUDP;
			}
		}

		return { currentMobHp_, score_ };
	}

private:
	// 1 bytes
	uint8_t roomNum;
	std::atomic<bool> finishCheck = false;
	std::atomic<uint8_t> startCheck = 0;

	// 4 bytes
	std::atomic<unsigned int> mobHp;

	// 8 bytes
	SOCKET* udpSkt;
	MatchingManager* matchingManager;
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now()+ std::chrono::minutes(2); // 생성 되자마자 삭제 방지

	// 32 bytes
	std::vector<RaidUserInfo> ruInfos;

	// 256 bytes
	char buffer[64];
};