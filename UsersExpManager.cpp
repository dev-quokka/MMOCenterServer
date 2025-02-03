#include "UsersExpManager.h"

void UsersExpManager::Init(UINT16 maxClientCount_) {
	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUser[i] = new EXP_MANAGER;
	}
}

uint8_t UsersExpManager::GetLevel(UINT16 connObjNum_) {
	return inGmaeUser[connObjNum_]->currentLevel;
}


void UsersExpManager::Set(UINT16 connObjNum_, uint8_t currentLevel_, unsigned int currentExp_) {
	inGmaeUser[connObjNum_]->currentLevel = currentLevel_;
	inGmaeUser[connObjNum_]->currentExp = currentExp_;
}

std::pair<uint8_t, unsigned int> UsersExpManager::ExpUp(UINT16 connObjNum_, short mobExp_) {
	inGmaeUser[connObjNum_]->currentExp += mobExp_;

	uint8_t levelUpCnt = 0;
	uint8_t currentLevel = inGmaeUser[connObjNum_]->currentLevel;
	unsigned int currentExp = inGmaeUser[connObjNum_]->currentExp;

	if (enhanceProbabilities[currentLevel] <= currentExp) { // LEVEL UP
		while (currentExp >= enhanceProbabilities[currentLevel]) {
			currentLevel++;
			levelUpCnt++;
		}
	}

	return { levelUpCnt , currentExp }; // Increase Level, Increase Exp
}

void UsersExpManager::Reset(UINT16 connObjNum_) {
	inGmaeUser[connObjNum_]->currentLevel = 0;
	inGmaeUser[connObjNum_]->currentExp = 0;
}