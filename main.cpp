#include "QuokkaServer.h"
const UINT16 PORT = 8080;
const UINT16 maxClientCount = 100;

#include <iostream>

int main() {

    QuokkaServer server(maxClientCount);

    UINT16 core= std::thread::hardware_concurrency();

    server.init(1,PORT);

    server.StartWork();

    std::cout << "if you exit, write quokka" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    return 0;
}



