#pragma once

#include <sw/redis++/redis++.h>
#include <iostream>
#include <windef.h>

class RedisManager {
public:
    void Run(const UINT16 RedisThreadCnt_);
    void EndRedisThreads(); // End Redis Threads

private:
    void ProcRedisPacket(); // Process Redis Packet
    void PushRedisPacket(); // Push Redis Packet

private:
    // 1 bytes
    bool redisRun = 0;

    // 8 bytes
    sw::redis::RedisCluster redis;

    // 32 bytes
    std::vector<std::thread> redisPool;

    // 72 bytes
    std::condition_variable cv;

    // 80 bytes
    std::mutex redisMu;

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;
};