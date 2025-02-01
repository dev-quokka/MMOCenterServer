#include "InGameUserManager.h"

void InGameUserManager::Init(UINT16 maxClientCount_) {
	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUser[i] = new EXP_MANAGER;
	}
}

void InGameUserManager::Set(UINT16 connObjNum_, uint8_t currentLevel_, unsigned int currentExp_) {
	inGmaeUser[connObjNum_]->currentLevel = currentLevel_;
	inGmaeUser[connObjNum_]->currentExp = currentExp_;
}

std::pair<uint8_t, unsigned int> InGameUserManager::ExpUp(UINT16 connObjNum_, unsigned int expUp_) {
	uint8_t currentLevel = inGmaeUser[connObjNum_]->currentLevel;
	unsigned int currentExp = inGmaeUser[connObjNum_]->currentExp;

	if (enhanceProbabilities[currentLevel] <= currentExp + expUp_) {
		currentExp += expUp_;
		while (1) {
			currentExp -= enhanceProbabilities[currentLevel];
			currentLevel += 1;
		}
	}
	else {
		currentLevel = 0;
		currentExp = currentExp + expUp_;
	}

	return { currentLevel , currentExp };
}

void InGameUserManager::Reset(UINT16 connObjNum_) {
	inGmaeUser[connObjNum_]->currentLevel = 0;
	inGmaeUser[connObjNum_]->currentExp = 0;
}