#include "RedisManager.h"

thread_local std::mt19937 RedisManager::gen(std::random_device{}());

void RedisManager::init(const UINT16 RedisThreadCnt_, const UINT16 maxClientCount_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::vector<RECV_PACKET_FUNCTION>(PACKET_ID_SIZE, nullptr);

    //SYSTEM
    packetIDTable[1] = &RedisManager::UserConnect;
    packetIDTable[2] = &RedisManager::Logout;
    packetIDTable[4] = &RedisManager::ServerEnd;

    // USER STATUS

    // INVENTORY
    packetIDTable[25] = &RedisManager::AddItem;
    packetIDTable[26] = &RedisManager::DeleteItem;
    packetIDTable[27] = &RedisManager::MoveItem;

    matchingManager = std::make_unique<MatchingManager>(maxClientCount_);
    inGameUserManager = std::make_unique<InGameUserManager>();
    roomManager = std::make_unique<RoomManager>();

    inGameUserManager->Init(maxClientCount_);

    RedisRun(RedisThreadCnt_);
    matchingManager->Init(maxClientCount_);
}

void RedisManager::RedisRun(const UINT16 RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        // Redis 클러스터 연결
        redis = sw::redis::RedisCluster(connection_options);
        std::cout << "Redis 클러스터 연결 성공!" << std::endl;

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis 에러 발생: " << err.what() << std::endl;
    }
}

void RedisManager::Disconnect(SOCKET userSkt) {
    this->UserDisConnect(userSkt);
}

void RedisManager::SetConnUserManager(ConnUsersManager* connUsersManager_) {
    connUsersManager = connUsersManager_;
}

bool RedisManager::CreateRedisThread(const UINT16 RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisThreads.emplace_back(std::thread([this]() {RedisThread(); }));
    }
    return true;
}

bool RedisManager::EquipmentEnhance(short currentEnhanceCount_) { // 
    if (currentEnhanceCount_ < 0 || currentEnhanceCount_ >= enhanceProbabilities.size()) { // Strange Enhance
        return false;
    }

    std::uniform_int_distribution<int> range(1, 100);
    return dist(gen) <= enhanceProbabilities[currentEnhanceCount_];
}

void RedisManager::RedisThread() {
    DataPacket tempD(0,0);
    ConnUser* TempConnUser = nullptr;
    char* tempData = nullptr;
    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            TempConnUser = connUsersManager->FindUser(tempD.userSkt);
            PacketInfo packetInfo = TempConnUser->ReadRecvData(tempData,tempD.dataSize); // GetData
            (this->*packetIDTable[packetInfo.packetId])(packetInfo.userSkt, packetInfo.dataSize,packetInfo.pData); // Proccess Packet
        }
        else { // Empty Queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void RedisManager::PushRedisPacket(const SOCKET userSkt_, const UINT32 size_, char* recvData_) {
    DataPacket tempD(size_,userSkt_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt_);
    TempConnUser->WriteRecvData(recvData_,size_); // Push Data in Circualr Buffer
    procSktQueue.push(tempD);
}

void RedisManager::SyncRaidScoreToRedis(RAID_END_REQUEST raidEndReqPacket1_, RAID_END_REQUEST raidEndReqPacket2_) {
    connUsersManager->FindUser(webServerSocket)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&raidEndReqPacket1_);
    connUsersManager->FindUser(webServerSocket)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&raidEndReqPacket2_);
}


// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void RedisManager::UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);

    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());
    tempUser->Set(userConn->uuId, userConn->userId, userConn->userPk, userConn->currentExp,userConn->level);

    redis.persist("user:" + userConn->uuId); // Remove TTL Time
}

void RedisManager::Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { // Normal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.uuId = tempUser->GetUuid();
        syncLogoutReqPacket.userPk = tempUser->GetPk();

        tempUser->Reset();
        connUsersManager->FindUser(webServerSocket)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
    }
}

void RedisManager::UserDisConnect(SOCKET userSkt) { // Abnormal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_DISCONNECT_REQUEST syncDisconnReqPacket;
        syncDisconnReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_DISCONNECT_REQUEST;
        syncDisconnReqPacket.PacketLength = sizeof(SYNCRONIZE_DISCONNECT_REQUEST);
        syncDisconnReqPacket.uuId = tempUser->GetPk();

        tempUser->Reset();
        connUsersManager->FindUser(webServerSocket)->PushSendMsg(sizeof(SYNCRONIZE_DISCONNECT_REQUEST), (char*)&syncDisconnReqPacket);
    }
}

void RedisManager::ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    // Process Remain Packet

}

void RedisManager::ImWebRequest(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    IM_WEB_RESPONSE imWebResPacket;
    imWebResPacket.PacketId = (UINT16)PACKET_ID::IM_WEB_RESPONSE;
    imWebResPacket.PacketLength = sizeof(IM_WEB_RESPONSE);
    imWebResPacket.uuId = tempUser->GetUuid();

    if (webServerSocket != 0) { // Web Server Already Exist
        imWebResPacket.isSuccess = false;
        connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
        return;
    }

    webServerSocket = userSkt;
    imWebResPacket.isSuccess = true;

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
}


//  ---------------------------- USER_STATUS  ----------------------------

void RedisManager::ExpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto expUpReqPacket = reinterpret_cast<EXP_UP_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    EXP_UP_RESPONSE expUpResPacket;
    expUpResPacket.PacketId = (UINT16)PACKET_ID::EXP_UP_RESPONSE;
    expUpResPacket.PacketLength = sizeof(EXP_UP_RESPONSE);
    expUpResPacket.uuId = tempUser->GetUuid();

    std::string user_slot = "userinfo:" + tempUser->GetUuid();

    if (redis.hincrby(user_slot, "exp", mobExp[expUpReqPacket->mobNum])) { // Exp Up Success
        auto userExp = tempUser->ExpUp(mobExp[expUpReqPacket->mobNum]); // Increase Level Cnt , Current Exp

        if (userExp.first!=0) { // Level Up
            LEVEL_UP_RESPONSE levelUpResPacket;
            levelUpResPacket.PacketId = (UINT16)PACKET_ID::LEVEL_UP_RESPONSE;
            levelUpResPacket.PacketLength = sizeof(LEVEL_UP_RESPONSE);
            levelUpResPacket.uuId = tempUser->GetUuid();

            if (redis.hincrby(user_slot, "level", userExp.first)) { // Level Up Success
                levelUpResPacket.increaseLevel = userExp.first;
                levelUpResPacket.currentExp = userExp.second;

                connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&levelUpResPacket);

                { // Send User PK, Level, Exp data to the Web Server for Synchronization with MySQL
                    ConnUser* TempWebServer = connUsersManager->FindUser(webServerSocket);

                    SYNCRONIZE_LEVEL_REQUEST syncLevelReqPacket;
                    syncLevelReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_LEVEL_REQUEST;
                    syncLevelReqPacket.PacketLength = sizeof(SYNCRONIZE_LEVEL_REQUEST);
                    syncLevelReqPacket.uuId = tempUser->GetUuid();
                    syncLevelReqPacket.level = userExp.first;
                    syncLevelReqPacket.currentExp = userExp.second;
                    syncLevelReqPacket.userPk = tempUser->GetPk();

                    TempWebServer->PushSendMsg(sizeof(SYNCRONIZE_LEVEL_REQUEST), (char*)&syncLevelReqPacket);
                }
            }
            else { // Level Up Fail
                levelUpResPacket.increaseLevel = 0;
                levelUpResPacket.currentExp = 0;
                connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&levelUpResPacket);
            }
        }
        else { // Just Exp Up
            expUpResPacket.expUp = userExp.second;
            connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
        }
    }
    else{ // Exp Up Fail
        expUpResPacket.expUp = 0;
        connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
    }
}


//  ---------------------------- INVENTORY  ----------------------------

void RedisManager::AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto addItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());
    
    ADD_ITEM_RESPONSE addItemResPacket;
    addItemResPacket.PacketId = (UINT16)PACKET_ID::ADD_ITEM_RESPONSE;
    addItemResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);
    addItemResPacket.uuId = tempUser->GetUuid();

    if (addItemReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hset(inventory_slot, itemType[addItemReqPacket->itemType] + std::to_string(addItemReqPacket->itemCode) + std::to_string(addItemReqPacket->itemSlotPos), std::to_string(addItemReqPacket->itemCount))) { // AddItem Success (ItemCode:slotposition, count)
            addItemResPacket.isSuccess = true;
        }
        else { // AddItem Fail
            addItemResPacket.isSuccess = false;
        }
    }
    else { // UUID NOT CORRECT
        addItemResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(ADD_ITEM_RESPONSE),(char*)&addItemResPacket);
}

void RedisManager::DeleteItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    DEL_ITEM_RESPONSE delItemResPacket;
    delItemResPacket.PacketId = (UINT16)PACKET_ID::DEL_ITEM_RESPONSE;
    delItemResPacket.PacketLength = sizeof(DEL_ITEM_RESPONSE);
    delItemResPacket.uuId = tempUser->GetUuid();

    if (delItemReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hdel(inventory_slot, itemType[delItemReqPacket->itemType] + std::to_string(delItemReqPacket->itemCode) + std::to_string(delItemReqPacket->itemSlotPos))) { // DeleteItem Success
            delItemResPacket.isSuccess = true;
        }
        else { // DeleteItem Fail
            delItemResPacket.isSuccess = false;
        }
    }
    else { // UUID NOT CORRECT
        delItemResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(DEL_ITEM_RESPONSE),(char*)&delItemResPacket);
}

void RedisManager::ModifyItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto modItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    MOD_ITEM_RESPONSE modItemResPacket;
    modItemResPacket.PacketId = (UINT16)PACKET_ID::MOD_ITEM_RESPONSE;
    modItemResPacket.PacketLength = sizeof(MOD_ITEM_RESPONSE);
    modItemResPacket.uuId = tempUser->GetUuid();

    if (modItemReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hset(inventory_slot, itemType[modItemReqPacket->itemType] + std::to_string(modItemReqPacket->itemCode) + std::to_string(modItemReqPacket->itemSlotPos), std::to_string(modItemReqPacket->itemCount))) { // ModifyItem Success
            modItemResPacket.isSuccess = true;
        }
        else { // ModifyItem Fail
            modItemResPacket.isSuccess = false;
        }
    }
    else { // UUID NOT CORRECT
        modItemResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(MOD_ITEM_RESPONSE), (char*)&modItemResPacket);
}

void RedisManager::MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto movItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    MOV_ITEM_RESPONSE movItemResPacket;
    movItemResPacket.PacketId = (UINT16)PACKET_ID::MOV_ITEM_RESPONSE;
    movItemResPacket.PacketLength = sizeof(MOV_ITEM_RESPONSE);
    movItemResPacket.uuId = tempUser->GetUuid();

    if (movItemReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hset(inventory_slot, itemType[movItemReqPacket->itemType] + std::to_string(movItemReqPacket->itemCode) + std::to_string(movItemReqPacket->itemSlotPos), std::to_string(movItemReqPacket->itemCount))) { // MoveItem Success
            movItemResPacket.isSuccess = true;
        }
        else { // MoveItem Fail
            movItemResPacket.isSuccess = false;
        }
    }
    else { // UUID NOT CORRECT
        movItemResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(MOV_ITEM_RESPONSE), (char*)&movItemResPacket);
}


//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

void RedisManager::AddEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto addEquipReqPacket = reinterpret_cast<ADD_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    ADD_EQUIPMENT_RESPONSE addEquipResPacket;
    addEquipResPacket.PacketId = (UINT16)PACKET_ID::ADD_EQUIPMENT_RESPONSE;
    addEquipResPacket.PacketLength = sizeof(ADD_EQUIPMENT_RESPONSE);
    addEquipResPacket.uuId = tempUser->GetUuid();

    if (addEquipReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hset(inventory_slot, itemType[addEquipReqPacket->itemType] + std::to_string(addEquipReqPacket->itemCode) + std::to_string(addEquipReqPacket->itemSlotPos), std::to_string(addEquipReqPacket->currentEnhanceCount))) { // AddItem Success (ItemCode:slotposition, count)
            addEquipResPacket.isSuccess = true;
        }
        else { // AddItem Fail
            addEquipResPacket.isSuccess = false;
        }
    }

    else { // UUID NOT CORRECT
        addEquipResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(ADD_EQUIPMENT_RESPONSE), (char*)&addEquipResPacket);
}

void RedisManager::DeleteEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<DEL_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    DEL_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::DEL_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(DEL_EQUIPMENT_RESPONSE);
    delEquipResPacket.uuId = tempUser->GetUuid();

    if (delEquipReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hdel(inventory_slot, itemType[delEquipReqPacket->itemType] + std::to_string(delEquipReqPacket->itemCode) + std::to_string(delEquipReqPacket->itemSlotPos))) { // DeleteItem Success
            delEquipResPacket.isSuccess = true;
        }
        else { // DeleteItem Fail
            delEquipResPacket.isSuccess = false;
        }
    }
    else { // UUID NOT CORRECT
        delEquipResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}

void RedisManager::EnhanceEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<ENH_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    ENH_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::ENH_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(ENH_EQUIPMENT_RESPONSE);
    delEquipResPacket.uuId = tempUser->GetUuid();

    if (delEquipReqPacket->uuId == tempUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + tempUser->GetUuid();

        if (redis.hdel(inventory_slot, 
            itemType[delEquipReqPacket->itemType] + std::to_string(delEquipReqPacket->itemCode) + std::to_string(delEquipReqPacket->itemSlotPos), std::to_string(delEquipReqPacket->currentEnhanceCount))) {
            
            if (EquipmentEnhance(delEquipReqPacket->currentEnhanceCount)) { // Enhance Success
                delEquipResPacket.isSuccess = true;
            }
            else { // Enhance Success
                delEquipResPacket.isSuccess = false;
            }

        }
        else { // DeleteItem Fail
            delEquipResPacket.isSuccess = false;
        }

    }
    else { // UUID NOT CORRECT
        delEquipResPacket.isSuccess = false;
    }

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}


//  ---------------------------- RAID  ----------------------------

void RedisManager::MatchStart(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { 
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    RAID_MATCHING_RESPONSE raidMatchResPacket;
    raidMatchResPacket.PacketId = (UINT16)PACKET_ID::RAID_RANKING_RESPONSE;
    raidMatchResPacket.PacketLength = sizeof(RAID_RANKING_RESPONSE);
    raidMatchResPacket.uuId = tempUser->GetUuid();

    if (matchingManager->Insert(tempUser->GetLevel(), userSkt, tempUser->GetId())) { // Insert Into Mathcing Queue Success
        raidMatchResPacket.insertSuccess = true;
    }
    else raidMatchResPacket.insertSuccess = false; // Mathing Queue Full

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&raidMatchResPacket);
}

void RedisManager::RaidReqTeamInfo(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);

    Room* tempRoom = roomManager->GetRoom(raidTeamInfoReqPacket->roomNum);
    InGameUser* user = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());
    InGameUser* teamUser = tempRoom->GetTeamUser(raidTeamInfoReqPacket->myNum);

    RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
    raidTeamInfoResPacket.PacketId = (UINT16)PACKET_ID::RAID_TEAMINFO_RESPONSE;
    raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
    raidTeamInfoResPacket.uuId = user->GetUuid();
    raidTeamInfoResPacket.teamLevel = teamUser->GetLevel();
    raidTeamInfoResPacket.teamId = teamUser->GetId();

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);

    if (tempRoom->StartCheck()) { // 두 명의 유저에게 팀의 정보를 전달하고 둘 다 받음 확인하면 게임 시작 정보 보내주기

        RAID_START_REQUEST raidStartReqPacket1;
        raidStartReqPacket1.PacketId = (UINT16)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket1.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket1.uuId = user->GetUuid();
        raidStartReqPacket1.endTime = tempRoom->GetEndTime();

        RAID_START_REQUEST raidStartReqPacket2;
        raidStartReqPacket2.PacketId = (UINT16)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket2.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket2.uuId = teamUser->GetUuid();
        raidStartReqPacket2.endTime = tempRoom->GetEndTime();

        connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket1);
        connUsersManager->FindUser(tempRoom->GetTeamSkt(raidTeamInfoReqPacket->myNum))->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket2);
    }
}

void RedisManager::RaidHit(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_HIT_REQUEST*>(pPacket_);
    InGameUser* user = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    roomManager->GetRoom(raidTeamInfoReqPacket->roomNum)->Hit(raidTeamInfoReqPacket->myNum, raidTeamInfoReqPacket->damage);


}

void RedisManager::RaidEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { // Send User Raid End Packet
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());
    
    //redis.zadd("user_scores", score, to_string(user_id));


}

void RedisManager::GetRaidScore(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<RAID_RANKING_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connUsersManager->FindUser(userSkt)->GetObjNum());

    std::vector<std::pair<std::string, unsigned int>> scores;
    redis.zrevrange("raidscore", delEquipReqPacket->startRank, delEquipReqPacket->startRank+99, inserter(scores, scores.begin()));

    RAID_RANKING_RESPONSE raidRankResPacket;
    raidRankResPacket.PacketId = (UINT16)PACKET_ID::RAID_RANKING_RESPONSE;
    raidRankResPacket.PacketLength = sizeof(RAID_RANKING_RESPONSE);
    raidRankResPacket.uuId = tempUser->GetUuid();
    raidRankResPacket.reqScore = scores;

    connUsersManager->FindUser(userSkt)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);
}