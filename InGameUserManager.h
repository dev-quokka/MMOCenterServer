#pragma once
#include "InGameUser.h"

#include <vector>
#include <string>
#include <ws2tcpip.h>
#include <utility>

class InGameUserManager {
public:
	void Init(UINT16 maxClientCount_);
	InGameUser* GetInGameUserByObjNum(UINT16 connObjNum_);
	void Set(std::string userUuid_, UINT32 userPk_, unsigned int userExp_, uint8_t userLevel);
	void Reset(UINT16 connObjNum_);

private:
	std::vector<InGameUser*> inGmaeUsers;
	std::vector<short> expLimit = { 1,1,2,3,5,8,13,21,34,56,90,146,236,382,618,1000 };
};