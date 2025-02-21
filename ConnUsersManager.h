#pragma once

#include <winsock2.h>
#include <iostream>
#include <vector>

#include "ConnUser.h"

class ConnUsersManager {
public:
    ~ConnUsersManager() {
        for (int i = 0; i < ConnUsers.size(); i++) {
            delete ConnUsers[i];
        }
    }

    void InsertUser(uint16_t connObjNum, ConnUser* connUser); // Init ConnUsers
    ConnUser* FindUser(uint16_t connObjNum);

private:
    // 576 bytes
    std::vector<ConnUser*> ConnUsers; // ConnUsers Obj
};
