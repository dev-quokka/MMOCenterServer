#pragma once

#include <sw/redis++/redis++.h>
#include <iostream>
#include <windef.h>

class RedisManager {
public:
    void Run(const UINT16 RedisThreadCnt_);
    void EndRedisThreads(); // End Redis Threads

private:
    void PushRedisPacket(); // Push Redis Packet
    bool CreateRedisThread(const UINT16 RedisThreadCnt_);
    void RedisThread();


    // 1 bytes
    bool redisRun = 0;

    // 8 bytes
    sw::redis::RedisCluster redis;

    // 16 bytes
    std::thread redisThread;

    // 32 bytes
    std::vector<std::thread> redisPool;

    // 72 bytes
    std::condition_variable cv;

    // 80 bytes
    std::mutex redisMu;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;
};