#pragma once

class InGameUser {
public:
	InGameUser(std::vector<short>* expLimit_) : expLimit(expLimit_) {}

	uint8_t GetLevel(UINT16 connObjNum_) {
		return userLevel;
	}

	void Set(std::string userUuid_, UINT32 userPk_, unsigned int userExp_, uint8_t userLevel_) {
		userLevel = userLevel_;
		userExp = userExp_;
		userPk = userPk_;
		userUuid = userUuid_;
	}

	void Reset() {
		userLevel = 0;
		userPk = 0;
		userExp = 0;
		userUuid = "";
	}

	UINT32 GetUserPk() {
		return userPk;
	}

	std::string GetUuid() {
		return userUuid;
	}

	std::pair<uint8_t, unsigned int> ExpUp(short mobExp_) {
		userExp += mobExp_;

		uint8_t levelUpCnt = 0;

		if ((*expLimit)[userLevel] <= userExp) { // LEVEL UP
			while (userExp >= (*expLimit)[userLevel]) {
				userLevel++;
				levelUpCnt++;
			}
		}

		return { levelUpCnt , userExp }; // Increase Level, Current Exp
	}

private:
	// 1 bytes
	uint8_t userLevel;

	// 4 bytes
	UINT32 userPk;
	unsigned int userExp;

	std::vector<short>* expLimit;

	// 40 bytes
	std::string userUuid;
};