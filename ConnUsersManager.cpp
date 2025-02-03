#include "ConnUsersManager.h"

void ConnUsersManager::InsertUser(SOCKET TempSkt_) {
	ConnUsers.insert({TempSkt_, nullptr});
};

ConnUser* ConnUsersManager::FindUser(SOCKET UserSkt_) {
	tbb::concurrent_hash_map<SOCKET, ConnUser*>::accessor accessor;
	if (ConnUsers.find(accessor, UserSkt_)) return accessor->second;
	else return nullptr;
};