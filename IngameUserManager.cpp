#include "InGameUserManager.h"

void InGameUserManager::Init(uint16_t maxClientCount_) {
	inGmaeUsers.resize(maxClientCount_, nullptr);

	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUsers[i] = new InGameUser(expLimit);
	}
}

void InGameUserManager::Set(uint16_t connObjNum_, std::string userUuid_, std::string userId_, uint32_t userPk_, unsigned int userExp_, uint16_t userLevel_) {
	inGmaeUsers[connObjNum_]->Set(userUuid_, userId_,userPk_, userExp_,userLevel_);
}

InGameUser* InGameUserManager::GetInGameUserByObjNum(uint16_t connObjNum_) {
	return inGmaeUsers[connObjNum_];
}

void InGameUserManager::Reset(uint16_t connObjNum_) {
	inGmaeUsers[connObjNum_]->Reset();
}