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
    bool redisRun = 0;
    std::vector<std::thread> redisPool;
    sw::redis::RedisCluster redis;
    std::condition_variable cv;
    std::mutex redisMu;
    sw::redis::ConnectionOptions connection_options;
};