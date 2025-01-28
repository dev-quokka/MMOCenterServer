#pragma once

#include "ConnUsersManager.h"
#include "Packet.h"

#include <mysql.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include <windef.h>
#include <unordered_map>

#pragma comment (lib, "libmysql.lib") // mysql 연동

class RedisManager {
public:
    void init(const UINT16 RedisThreadCnt_);
    void EndRedisThreads(); // End Redis Threads
    void SetConnUserManager(ConnUsersManager* connUsersManager_);
    void PushRedisPacket(const SOCKET userSkt, const UINT32 size_, char* recvData_); // Push Redis Packet
    void Disconnect(SOCKET userSkt);

private:
    void RedisRun(const UINT16 RedisThreadCnt_);
    void MysqlRun();
    void SendMsg(SOCKET TempSkt_);
    bool CreateRedisThread(const UINT16 RedisThreadCnt_);
    void RedisThread();
    void CloseMySQL(); // Close mysql

    //SYSTEM
    void UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // User Connect

    void Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // Normal Disconnect

    void UserDisConnect(SOCKET userSkt); // Abnormal Disconnect

    void ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // USER STATUS

    // INVENTORY
    void AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void DeleteItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void ModifyItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // 1 bytes
    bool redisRun = 0;

    // 8 bytes
    sw::redis::RedisCluster redis;
    ConnUsersManager* connUsersManager;

    // 16 bytes
    std::thread redisThread;

    // 32 bytes
    typedef void(RedisManager::*RECV_PACKET_FUNCTION)(SOCKET, UINT16, char*); 
    std::vector<RECV_PACKET_FUNCTION> packetIDTable;
    std::vector<std::thread> redisPool;
    
    // 72 bytes
    std::condition_variable cv;

    // 80 bytes
    std::mutex redisMu;

    // 104 bytes
    MYSQL_RES* Result;

    // 136 bytes 
    boost::lockfree::queue<DataPacket> procSktQueue;// 나중에 병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 1096 bytes
    MYSQL Conn;
    MYSQL* ConnPtr = NULL;
};