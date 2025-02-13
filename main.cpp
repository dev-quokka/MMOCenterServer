#include "QuokkaServer.h"

const uint16_t PORT = 8080;
const uint16_t maxClientCount = 100;

#include <iostream>
#include <cstdint>

int main() {

    QuokkaServer server(maxClientCount);

    uint16_t core= std::thread::hardware_concurrency();

    server.init(1,PORT);

    server.StartWork();

    std::cout << "If You Want Exit, Write quokka" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "quokka") break;
    }

    server.ServerEnd();

    return 0;
}



