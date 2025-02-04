#pragma once

class InGameUser {
public:
	InGameUser(std::vector<short>* expLimit_) : expLimit(expLimit_) {}

	uint8_t GetLevel(UINT16 connObjNum_) {
		return currentLevel;
	}

	void Set(std::string userUuid_, UINT32 userPk_, unsigned int userExp_, uint8_t userLevel) {

	}

	void Reset() {
		currentLevel = 0;
		userPk = 0;
		currentExp = 0;
		userUuid = "";
	}

	UINT32 GetPk() {
		return userPk;
	}

	std::string GetUuid() {
		return userUuid;
	}

	std::pair<uint8_t, unsigned int> ExpUp(short mobExp_) {
		currentExp += mobExp_;

		uint8_t levelUpCnt = 0;
		uint8_t currentLevel = currentLevel;
		unsigned int currentExp = currentExp;

		if ((*expLimit)[currentLevel] <= currentExp) { // LEVEL UP
			while (currentExp >= (*expLimit)[currentLevel]) {
				currentLevel++;
				levelUpCnt++;
			}
		}

		return { levelUpCnt , currentExp }; // Increase Level, Increase Exp
	}

private:
	// 1 bytes
	uint8_t currentLevel;

	// 4 bytes
	UINT32 userPk;
	unsigned int currentExp;

	std::vector<short>* expLimit;

	// 40 bytes
	std::string userUuid;
};