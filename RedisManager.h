#pragma once

#include <winsock2.h>
#include <windef.h>
#include <cstdint>
#include <iostream>
#include <random>
#include <unordered_map>
#include <sw/redis++/redis++.h>

#include "Packet.h"
#include "InGameUser.h"
#include "InGameUserManager.h"
#include "RoomManager.h"
#include "MatchingManager.h"
#include "ConnUsersManager.h"

class RoomManager;
class MatchingManager;

class RedisManager {
public:
    ~RedisManager() {
        redisRun = false;

        for (int i = 0; i < redisThreads.size(); i++) { // End Redis Threads
            if (redisThreads[i].joinable()) {
                redisThreads[i].join();
            }
        }
    }

    void init(const uint16_t RedisThreadCnt_, const uint16_t maxClientCount_, const HANDLE sIOCPHandle_);
    void SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_, MatchingManager* matchingManager_);
    void PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_); // Push Redis Packet
    void Disconnect(uint16_t connObjNum_);

    // Send Data to Web Server for Synchronization With Redis
    void SyncRaidScoreToRedis(RAID_END_REQUEST raidEndReqPacket1, RAID_END_REQUEST raidEndReqPacket2);

private:
    bool CreateRedisThread(const uint16_t RedisThreadCnt_);
    bool EquipmentEnhance(short currentEnhanceCount_);

    void RedisRun(const uint16_t RedisThreadCnt_);
    void RedisThread();

    //SYSTEM
    void UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // User Connect
    void Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Normal Disconnect (Set Short Time TTL)
    void UserDisConnect(uint16_t connObjNum_); // Abnormal Disconnect (Set Long Time TTL)
    void ServerEnd(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ImWebRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // Web Server Socket Check

    // USER STATUS
    void ExpUp(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // INVENTORY
    void AddItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void DeleteItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void ModifyItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void MoveItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // INVENTORY:EQUIPMENT
    void AddEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void DeleteEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);
    void EnhanceEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    // RAID
    void MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // ��ġ ��⿭ ����
    void RaidReqTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_); // ���� �� ���� ��û (��� ��� Ȯ��)

    void RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);

    void GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_);


    // 1 bytes
    bool redisRun = false;

    // 8 bytes
    uint16_t webServerObjNum = 0;
    std::unique_ptr<sw::redis::RedisCluster> redis;
    std::uniform_int_distribution<int> dist;

    // 16 bytes
    std::thread redisThread;

    // 32 bytes
    typedef void(RedisManager::*RECV_PACKET_FUNCTION)(uint16_t, uint16_t, char*);
    std::unordered_map<uint16_t, RECV_PACKET_FUNCTION> packetIDTable;
    std::vector<std::thread> redisThreads;

    std::vector<short> enhanceProbabilities = {100,90,80,70,60,50,40,30,20,10};
    std::vector<unsigned int> mobExp = { 0,1,2,3,4,5,6,7,8,9,10 };
    std::vector<std::string> itemType = {"equipment", "consumables", "materials" };

    ConnUsersManager* connUsersManager;

    // 64 bytes
    InGameUserManager* inGameUserManager;

    // 80 bytes
    std::mutex redisMu;
    RoomManager* roomManager;

    // 136 bytes 
    boost::lockfree::queue<DataPacket> procSktQueue{1024}; // ���߿� �������� �߻��ϸ� lock_guard,mutex ��� �Ǵ� lockfree::queue�� ũ�⸦ �ø��� ������� ��ȯ

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 936 bytes
    MatchingManager* matchingManager;

    // 5000 bytes
    thread_local static std::mt19937 gen;
};