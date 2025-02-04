#include "InGameUserManager.h"

void InGameUserManager::Init(UINT16 maxClientCount_) {
	inGmaeUsers.resize(maxClientCount_, nullptr);

	for (int i = 0; i < maxClientCount_; i++) {
		inGmaeUsers[i] = new InGameUser(&expLimit);
	}
}


void InGameUserManager::Set(std::string userUuid_, UINT32 userPk_, unsigned int userExp_, uint8_t userLevel) {

}

InGameUser* InGameUserManager::GetInGameUserByObjNum(UINT16 connObjNum_) {
	return inGmaeUsers[connObjNum_];
}

void InGameUserManager::Reset(UINT16 connObjNum_) {
}