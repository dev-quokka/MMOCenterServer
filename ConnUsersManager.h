#pragma once

#include "ConnUser.h"

#include <tbb/concurrent_hash_map.h>

class ConnUsersManager {
public:
    void InsertUser(SOCKET TempSkt_); // Init ConnUsers
    void DeleteUser(SOCKET TempSkt_);
    ConnUser* FindUser(SOCKET UserSkt_);

private:
    // 576 bytes
    tbb::concurrent_hash_map<SOCKET, ConnUser*> ConnUsers; // ConnUsers Info
};
