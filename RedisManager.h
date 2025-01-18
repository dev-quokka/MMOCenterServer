#pragma once

#include <mysql.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <windef.h>

#pragma comment (lib, "libmysql.lib") // mysql 연동

class RedisManager {
public:
    void RedisRun(const UINT16 RedisThreadCnt_);
    void MysqlRun();
    void EndRedisThreads(); // End Redis Threads

private:
    void PushRedisPacket(); // Push Redis Packet
    bool CreateRedisThread(const UINT16 RedisThreadCnt_);
    void RedisThread();
    void CloseMySQL(); // Close mysql

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

    // 104 bytes
    MYSQL_RES* Result;

    // 136 bytes 
    // boost::lockfree::queue<> // 나중에 병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 1096 bytes
    MYSQL Conn;
    MYSQL* ConnPtr = NULL;
};