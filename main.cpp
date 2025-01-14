#include "QuokkaServer.h"
const UINT16 PORT = 8080;
const UINT32 UserCnt = 100;

#include <iostream>

int main() {

    QuokkaServer server;

    UINT16 core= std::thread::hardware_concurrency();

    server.init(core/2,PORT);

    server.StartWork(UserCnt);

    std::cout << "if you exit, write quokka" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    return 0;
}



