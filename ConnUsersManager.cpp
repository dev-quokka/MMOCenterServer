#include "ConnUsersManager.h"

void ConnUsersManager::InsertUser(ConnUser* connUser) {
	tbb::concurrent_hash_map<SOCKET, ConnUser*>::accessor accessor;
	if (ConnUsers.insert(accessor, connUser->GetSocket())) {
		accessor->second = connUser;
	}
};

void ConnUsersManager::DeleteUser(SOCKET TempSkt_){
	ConnUsers.erase(TempSkt_);
}

ConnUser* ConnUsersManager::FindUser(SOCKET UserSkt_) {
	tbb::concurrent_hash_map<SOCKET, ConnUser*>::accessor accessor;
	if (ConnUsers.find(accessor, UserSkt_)) return accessor->second;
	else return nullptr;
};