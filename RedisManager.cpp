#include "RedisManager.h"

void RedisManager::Run(const UINT16 RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        // Redis Ŭ������ ����
        redis = sw::redis::RedisCluster(connection_options);
        std::cout << "Redis Ŭ������ ���� ����!" << std::endl;

        redisRun = 1;

        for (int i = 0; i < RedisThreadCnt_; i++) {
            redisPool.emplace_back(std::thread([this]() {ProcRedisPacket();}));
        }

    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis ���� �߻�: " << err.what() << std::endl;
    }
}

void RedisManager::ProcRedisPacket() { 
    while (redisRun) {
        std::unique_lock<std::mutex> lock(redisMu);
        
    }
}

void RedisManager::PushRedisPacket() {
    

};