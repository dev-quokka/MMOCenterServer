#include "InGameUserManager.h"

void InGameUserManager::Init(UINT16 maxClientCount_) {
	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUser[i] = new EXP_MANAGER;
	}
}

uint8_t InGameUserManager::GetLevel(UINT16 connObjNum_) {
	return inGmaeUser[connObjNum_]->currentLevel;
}

UINT32 InGameUserManager::GetPk(UINT16 connObjNum_){
	return inGmaeUser[connObjNum_]->userPk;
}

void InGameUserManager::Set(UINT16 connObjNum_, UINT32 userPk_, uint8_t currentLevel_, unsigned int currentExp_) {
	inGmaeUser[connObjNum_]->currentLevel = currentLevel_;
	inGmaeUser[connObjNum_]->userPk = userPk_;
	inGmaeUser[connObjNum_]->currentExp = currentExp_;
}

std::pair<uint8_t, unsigned int> InGameUserManager::ExpUp(UINT16 connObjNum_, short mobExp_) {
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

void InGameUserManager::Reset(UINT16 connObjNum_) {
	inGmaeUser[connObjNum_]->currentLevel = 0;
	inGmaeUser[connObjNum_]->userPk = 0;
	inGmaeUser[connObjNum_]->currentExp = 0;
}