#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "Define.h"
#include "UdpOverLappedManager.h"

class MatchingManager;
class InGameUser;

struct RaidUserInfo {
	std::atomic<unsigned int> userScore = 0;
	uint16_t userObjNum; // TCP Socket
	sockaddr_in userAddr;
	InGameUser* inGameUser;
};

class Room {
public:
	Room(SOCKET* udpSkt_, UdpOverLappedManager* udpOverLappedManager_) {
		RaidUserInfo* ruInfo1 = new RaidUserInfo;
		ruInfos.emplace_back(ruInfo1);

		RaidUserInfo* ruInfo2 = new RaidUserInfo;
		ruInfos.emplace_back(ruInfo2);

		udpSkt = udpSkt_;
		udpOverLappedManager = udpOverLappedManager_;
	}
	~Room() {
		for (int i = 0; i < ruInfos.size(); i++) {
			delete ruInfos[i];
		}
	}

	void set(uint16_t roomNum_, uint16_t timer_, int mobHp_, uint16_t userObjNum1_, uint16_t userObjNum2_, InGameUser* user1_, InGameUser* user2_) {
		ruInfos[0]->userObjNum = userObjNum1_;
		ruInfos[0]->inGameUser = user1_;

		ruInfos[1]->userObjNum = userObjNum2_;
		ruInfos[1]->inGameUser = user2_;

		mobHp.store(mobHp_);
	}

	void setSockAddr(uint16_t userNum_, sockaddr_in userAddr_) {
		ruInfos[userNum_]->userAddr = userAddr_;
	}

	bool StartCheck() {
		if (startCheck.fetch_add(1) + 1 == 2) {
			endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2)+ std::chrono::seconds(8);
			return true;
		}
		return false;
	}

	bool EndCheck() {
		if (startCheck.fetch_sub(1) - 1 == 0) {
			return true;
		}
		return false;
	}

	uint16_t GetRoomNum() {
		return roomNum;
	}

	uint16_t GetRoomUserCnt() {
		return ruInfos.size();
	}

	InGameUser* GetUser(uint16_t userNum_) {
		if (userNum_ == 0) return ruInfos[0]->inGameUser;
		else if (userNum_ == 1) return ruInfos[1]->inGameUser;
	}

	std::chrono::time_point<std::chrono::steady_clock> GetEndTime() {
		endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2);
		return endTime;
	}

	SOCKET GetUserObjNum(uint16_t userNum) {
		if (userNum == 0) return ruInfos[0]->userObjNum;
		else if (userNum == 1) return ruInfos[1]->userObjNum;
	}

	unsigned int GetScore(uint16_t userNum) {
		if (userNum == 0) return ruInfos[0]->userScore;
		else if (userNum == 1) return ruInfos[1]->userScore;
	}

	SOCKET GetTeamObjNum(uint16_t userNum_) {
		if (userNum_ == 1) return ruInfos[0]->userObjNum;
		else if (userNum_ == 0) return ruInfos[1]->userObjNum;
	}

	InGameUser* GetTeamUser(uint16_t userNum_) {
		if (userNum_ == 1) return ruInfos[0]->inGameUser;
		else if (userNum_ == 0) return ruInfos[1]->inGameUser;
	}

	unsigned int GetTeamScore(uint16_t userNum) {
		if (userNum == 1) return ruInfos[0]->userScore;
		else if (userNum == 0) return ruInfos[1]->userScore;
	}

	std::pair<unsigned int, unsigned int> Hit(uint16_t userNum_, unsigned int damage_){ // current mobhp, score
		if (mobHp <= 0 || finishCheck.load()) {
			return {0,0};
		}
		
		unsigned int score_;
		int currentMobHp_;

		if ((currentMobHp_ = mobHp.fetch_sub(damage_) - damage_)<=0) { // Hit
			finishCheck.store(true);
			std::cout << "LAST hp " << currentMobHp_ << std::endl;
			std::cout << "LAST da" << damage_ << std::endl;
			std::cout << "LAST " << currentMobHp_ + damage_<< std::endl;
			score_ = ruInfos[userNum_]->userScore.fetch_add(currentMobHp_ + damage_) + (currentMobHp_ + damage_);
			std::cout << "�� �̹� ����. ������ ���ھ� ����" << std::endl;
			return { 0, score_ };
		}

		score_ = ruInfos[userNum_]->userScore.fetch_add(damage_) + damage_;
		
		for (int i = 0; i < ruInfos.size(); i++) { // ������ �����鿡�Ե� �ٲ� �� hp�� �����ֱ�

			OverlappedUDP* overlappedUDP = udpOverLappedManager->getOvLap();

			if (overlappedUDP == nullptr) { // ������ Ǯ�� ���� ������ ���� ������ ����
				OverlappedUDP* overlappedUDP = new OverlappedUDP;
				ZeroMemory(overlappedUDP, sizeof(OverlappedUDP));
				overlappedUDP->wsaBuf.len = sizeof(currentMobHp_);
				overlappedUDP->wsaBuf.buf = new char[sizeof(currentMobHp_)];
				CopyMemory(overlappedUDP->wsaBuf.buf, &currentMobHp_, sizeof(currentMobHp_));
				overlappedUDP->addrSize = sizeof(ruInfos[i]->userAddr);
				overlappedUDP->userAddr = ruInfos[i]->userAddr;
				overlappedUDP->taskType = TaskType::NEWSEND;

				DWORD dwSendBytes = 0;
				int result = WSASendTo(*udpSkt, &overlappedUDP->wsaBuf, 1, &dwSendBytes, 0, (SOCKADDR*)&ruInfos[i]->userAddr, sizeof(ruInfos[i]->userAddr), (LPWSAOVERLAPPED)overlappedUDP, NULL);

				std::cout << "���� ���� ���ھ� ����" << std::endl;
				std::cout <<"���� �� HP : " << mobHp << std::endl;
				if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
					std::cerr << "WSASendTo Fail : " << WSAGetLastError() << std::endl;
				}
			}
			else {
				overlappedUDP->wsaBuf.len = sizeof(currentMobHp_);
				overlappedUDP->wsaBuf.buf = new char[sizeof(currentMobHp_)];
				CopyMemory(overlappedUDP->wsaBuf.buf, &currentMobHp_, sizeof(currentMobHp_));
				overlappedUDP->addrSize = sizeof(ruInfos[i]->userAddr);
				overlappedUDP->userAddr = ruInfos[i]->userAddr;
				overlappedUDP->taskType = TaskType::SEND;

				DWORD dwSendBytes = 0;
				int result = WSASendTo(*udpSkt, &overlappedUDP->wsaBuf, 1, &dwSendBytes, 0, (SOCKADDR*)&ruInfos[i]->userAddr, sizeof(ruInfos[i]->userAddr), (LPWSAOVERLAPPED)overlappedUDP, NULL);

				std::cout << "���ھ� ����" << std::endl;
				std::cout << "���� �� HP : " << mobHp << std::endl;
				if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
					std::cerr << "WSASendTo Fail : " << WSAGetLastError() << std::endl;
				}
			}
		}

		return { currentMobHp_, score_ };
	}

private:
	// 1 bytes
	
	std::atomic<bool> finishCheck = false;
	std::atomic<uint16_t> startCheck = 0;

	// 2 bytes
	uint16_t roomNum;

	// 4 bytes
	std::atomic<int> mobHp;

	// 8 bytes
	SOCKET* udpSkt;
	MatchingManager* matchingManager;
	UdpOverLappedManager* udpOverLappedManager;
	std::chrono::time_point<std::chrono::steady_clock> endTime = std::chrono::steady_clock::now() + std::chrono::minutes(2); // ���� ���ڸ��� ���� ����

	// 32 bytes
	std::vector<RaidUserInfo*> ruInfos;
};