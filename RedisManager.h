#pragma once

#define MYSQL_USER "quokka"
#define MYSQL_PASSWORD "1234"
#define MYSQL_HOST "127.0.0.1"
#define MYSQL_DB "Quokka"

#include "ConnUsersManager.h"
#include "Packet.h"
#include "InGameUserManager.h"

#include <sw/redis++/redis++.h>
#include <json/json.h>
#include <mysql.h>
#include <windef.h>
#include <iostream>
#include <random>
#include <unordered_map>

#pragma comment (lib, "libmysql.lib") // mysql 연동

class RedisManager {
public:
    void init(const UINT16 RedisThreadCnt_, const UINT16 maxClientCount_);
    void EndRedisThreads(); // End Redis Threads
    void SetConnUserManager(ConnUsersManager* connUsersManager_);
    void PushRedisPacket(const SOCKET userSkt, const UINT32 size_, char* recvData_); // Push Redis Packet
    void Disconnect(SOCKET userSkt);

private:
    bool CreateRedisThread(const UINT16 RedisThreadCnt_);
    bool EquipmentEnhance(short currentEnhanceCount_);

    void RedisRun(const UINT16 RedisThreadCnt_);
    void MysqlRun();
    void SendMsg(SOCKET TempSkt_);
    void RedisThread();
    void CloseMySQL(); // Close mysql

    //SYSTEM
    void UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // User Connect
    void Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // Normal Disconnect (Set Short Time TTL)
    void UserDisConnect(SOCKET userSkt); // Abnormal Disconnect (Set Long Time TTL)
    void ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // USER STATUS
    void ExpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // INVENTORY
    void AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void DeleteItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void ModifyItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // INVENTORY:EQUIPMENT
    void AddEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void DeleteEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void EnhanceEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    // RAID


    // 1 bytes
    bool redisRun = 0;

    // 4 bytes
    int MysqlResult;

    // 8 bytes
    sw::redis::RedisCluster redis;
    MYSQL_ROW Row;
    std::uniform_int_distribution<int> dist;

    // 16 bytes
    std::thread redisThread;

    // 32 bytes
    typedef void(RedisManager::*RECV_PACKET_FUNCTION)(SOCKET, UINT16, char*); 
    std::vector<RECV_PACKET_FUNCTION> packetIDTable;
    std::vector<std::thread> redisPool;
    std::vector<std::string> itemType;
    std::vector<short> enhanceProbabilities = {100,90,80,70,60,50,40,30,20,10};

    // 64b bytes
    InGameUserManager* inGameUserManager;

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

    // 606 bytes
    ConnUsersManager* connUsersManager;

    // 1096 bytes
    MYSQL Conn;
    MYSQL* ConnPtr = NULL;

    // 5000 bytes
    thread_local static std::mt19937 gen;
};