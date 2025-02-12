#pragma once

#include "ConnUsersManager.h"
#include "Packet.h"
#include "InGameUserManager.h"
#include "MatchingManager.h"

#include <sw/redis++/redis++.h>
#include <windef.h>
#include <iostream>
#include <random>
#include <unordered_map>

class RedisManager {
public:
    void init(const UINT16 RedisThreadCnt_, const UINT16 maxClientCount_, const HANDLE sIOCPHandle_);
    void SetConnUserManager(ConnUsersManager* connUsersManager_);
    void PushRedisPacket(const SOCKET userSkt, const UINT32 size_, char* recvData_); // Push Redis Packet
    void Disconnect(SOCKET userSkt);

    // Send Data to Web Server for Synchronization With Redis
    void SyncRaidScoreToRedis(RAID_END_REQUEST raidEndReqPacket1, RAID_END_REQUEST raidEndReqPacket2);

private:
    bool CreateRedisThread(const UINT16 RedisThreadCnt_);
    bool EquipmentEnhance(short currentEnhanceCount_);

    void RedisRun(const UINT16 RedisThreadCnt_);
    void RedisThread();

    //SYSTEM
    void UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // User Connect
    void Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // Normal Disconnect (Set Short Time TTL)
    void UserDisConnect(SOCKET userSkt); // Abnormal Disconnect (Set Long Time TTL)
    void ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void ImWebRequest(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // Web Server Socket Check

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
    void MatchStart(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // 매치 대기열 삽입
    void RaidReqTeamInfo(SOCKET userSkt, UINT16 packetSize_, char* pPacket_); // 서로 팀 정보 요청 (상대 대기 확인)

    void RaidHit(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);

    void RaidEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);
    void GetRaidScore(SOCKET userSkt, UINT16 packetSize_, char* pPacket_);


    // 1 bytes
    bool redisRun = 0;

    // 8 bytes
    SOCKET webServerSkt = 0;
    sw::redis::RedisCluster redis;
    std::uniform_int_distribution<int> dist;

    std::unique_ptr<InGameUserManager> inGameUserManager;
    std::unique_ptr<MatchingManager> matchingManager;
    std::unique_ptr<RoomManager> roomManager;

    // 16 bytes
    std::thread redisThread;

    // 32 bytes
    typedef void(RedisManager::*RECV_PACKET_FUNCTION)(SOCKET, UINT16, char*); 
    std::vector<RECV_PACKET_FUNCTION> packetIDTable;
    std::vector<std::thread> redisThreads;

    std::vector<short> enhanceProbabilities = {100,90,80,70,60,50,40,30,20,10};
    std::vector<unsigned int> mobExp = { 0,1,2,3,4,5,6,7,8,9,10 };
    std::vector<std::string> itemType = {"equipment", "consumables", "materials" };

    // 72 bytes
    std::condition_variable cv;

    // 80 bytes
    std::mutex redisMu;

    // 136 bytes 
    boost::lockfree::queue<DataPacket> procSktQueue{1024}; // 나중에 병목현상 발생하면 lock_guard,mutex 사용 또는 lockfree::queue의 크기를 늘리는 방법으로 전환

    // 242 bytes
    sw::redis::ConnectionOptions connection_options;

    // 606 bytes
    ConnUsersManager* connUsersManager;

    // 5000 bytes
    thread_local static std::mt19937 gen;
};