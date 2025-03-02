#include "RedisManager.h"

thread_local std::mt19937 RedisManager::gen(std::random_device{}());

void RedisManager::init(const uint16_t RedisThreadCnt_, const uint16_t maxClientCount_, HANDLE sIOCPHandle_) {

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    //SYSTEM
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::USER_LOGOUT_REQUEST] = &RedisManager::Logout;
    packetIDTable[(uint16_t)PACKET_ID::IM_WEB_REQUEST] = &RedisManager::ImWebRequest;

    // USER STATUS
    packetIDTable[(UINT16)PACKET_ID::EXP_UP_REQUEST] = &RedisManager::ExpUp;

    // INVENTORY
    packetIDTable[(uint16_t)PACKET_ID::ADD_ITEM_REQUEST] = &RedisManager::AddItem;
    packetIDTable[(uint16_t)PACKET_ID::DEL_ITEM_REQUEST] = &RedisManager::DeleteItem;
    packetIDTable[(uint16_t)PACKET_ID::MOD_ITEM_REQUEST] = &RedisManager::ModifyItem;
    packetIDTable[(uint16_t)PACKET_ID::MOV_ITEM_REQUEST] = &RedisManager::MoveItem;

    //INVENTORY:EQUIPMENT
    packetIDTable[(uint16_t)PACKET_ID::ADD_EQUIPMENT_REQUEST] = &RedisManager::AddEquipment;
    packetIDTable[(uint16_t)PACKET_ID::DEL_EQUIPMENT_REQUEST] = &RedisManager::DeleteEquipment;
    packetIDTable[(uint16_t)PACKET_ID::ENH_EQUIPMENT_REQUEST] = &RedisManager::EnhanceEquipment;
    packetIDTable[(uint16_t)PACKET_ID::MOV_EQUIPMENT_REQUEST] = &RedisManager::MoveEquipment;

    //RAID
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::RAID_TEAMINFO_REQUEST] = &RedisManager::RaidReqTeamInfo;
    packetIDTable[(uint16_t)PACKET_ID::RAID_HIT_REQUEST] = &RedisManager::RaidHit;
    packetIDTable[(uint16_t)PACKET_ID::RAID_RANKING_REQUEST] = &RedisManager::GetRanking;

    RedisRun(RedisThreadCnt_);
}

void RedisManager::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connect Success !" << std::endl;

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
}

void RedisManager::Disconnect(uint16_t connObjNum_) {
    UserDisConnect(connObjNum_);
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_, RoomManager* roomManager_, MatchingManager* matchingManager_) {
    connUsersManager = connUsersManager_;
    inGameUserManager = inGameUserManager_;
    roomManager = roomManager_; 
    matchingManager = matchingManager_;
}

bool RedisManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisThreads.emplace_back(std::thread([this]() {RedisThread(); }));
    }
    return true;
}

bool RedisManager::EquipmentEnhance(short currentEnhanceCount_) {
    if (currentEnhanceCount_ < 0 || currentEnhanceCount_ >= enhanceProbabilities.size()) { // Strange Enhance
        return false;
    }

    std::uniform_int_distribution<int> range(1, 100);
    return dist(gen) <= enhanceProbabilities[currentEnhanceCount_];
}

void RedisManager::RedisThread() {
    DataPacket tempD(0,0);
    ConnUser* TempConnUser = nullptr;
    char tempData[1024] = {0};
    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            std::memset(tempData, 0, sizeof(tempData));
            TempConnUser = connUsersManager->FindUser(tempD.connObjNum); // Find User
            PacketInfo packetInfo = TempConnUser->ReadRecvData(tempData, tempD.dataSize); // GetData
            (this->*packetIDTable[packetInfo.packetId])(packetInfo.connObjNum, packetInfo.dataSize, packetInfo.pData); // Proccess Packet
        }
        else { // Empty Queue
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_,size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}

// ============================== PACKET ==============================

//  ---------------------------- SYSTEM  ----------------------------

void RedisManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);

    std::string key = "jwtcheck:{" + (std::string)userConn->userId + "}";
    auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));

    std::string userInfokey = "userinfo:{" + std::to_string(pk) + "}";
    std::unordered_map<std::string, std::string> userData;
    redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

    connUsersManager->FindUser(connObjNum_)->SetPk(pk);
    inGameUserManager->Set(connObjNum_, (std::string)userConn->userId, pk, std::stoul(userData["exp"]), static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]));

    USER_CONNECT_RESPONSE_PACKET ucReq;
    ucReq.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_RESPONSE;
    ucReq.PacketLength = sizeof(USER_CONNECT_RESPONSE_PACKET);
    ucReq.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
    std::cout << (std::string)userConn->userId << " Connect" << std::endl;
}

void RedisManager::Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { // Normal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(webServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
        std::cout << "���� �α׾ƿ� ��ũ�� �޽��� ����" << std::endl;
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(webServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
        std::cout << "���� ��Ŀ��Ʈ ��ũ�� �޽��� ����" << std::endl;
    }
}

void RedisManager::ServerEnd(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    // Process Remain Packet

}

void RedisManager::ImWebRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<IM_WEB_REQUEST*>(pPacket_);
    std::cout << "WebServer Connect Request :" << connObjNum_ << std::endl;

    IM_WEB_RESPONSE imWebResPacket;
    imWebResPacket.PacketId = (uint16_t)PACKET_ID::IM_WEB_RESPONSE;
    imWebResPacket.PacketLength = sizeof(IM_WEB_RESPONSE);

    std::string str(userConn->webToken);

    uint32_t pk;

    if (pk = static_cast<uint32_t>(std::stoul(*redis->hget("jwtcheck:{webserver}", str)))==0 ) { // Find JWT Fail(Protect From DDOS(�ѹ� �ƴ϶�� �Ǹ�Ǹ� ���� ���������))
        imWebResPacket.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
        return;
    }

    connUsersManager->FindUser(connObjNum_)->SetPk(pk);
    webServerObjNum = connObjNum_;
    imWebResPacket.isSuccess = true;
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
    std::cout << "WebServer Connect Success : " << connObjNum_ << std::endl;
}


//  ---------------------------- USER_STATUS  ----------------------------

void RedisManager::ExpUp(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto expUpReqPacket = reinterpret_cast<EXP_UP_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    std::string key = "userinfo:{" +  std::to_string(tempUser->GetPk()) + "}";

        auto userExp = tempUser->ExpUp(mobExp[expUpReqPacket->mobNum]); // Increase Level Cnt , Current Exp

        EXP_UP_RESPONSE expUpResPacket;
        expUpResPacket.PacketId = (uint16_t)PACKET_ID::EXP_UP_RESPONSE;
        expUpResPacket.PacketLength = sizeof(EXP_UP_RESPONSE);

        if (userExp.first!=0) { // Level Up
            auto pipe = redis->pipeline(std::to_string(tempUser->GetPk()));

            pipe.hset(key, "exp", std::to_string(userExp.second))
                .hincrby(key, "level", userExp.first);

            pipe.exec();

            expUpResPacket.increaseLevel = userExp.first;
            expUpResPacket.currentExp = userExp.second;

            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&expUpResPacket);
        }
        else { // Just Exp Up
            if (redis->hincrby(key, "exp", userExp.second)) { // Exp Up Success
                expUpResPacket.increaseLevel = userExp.first;
                expUpResPacket.currentExp = userExp.second;
                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
            }
            else { // Exp Up Fail
                expUpResPacket.increaseLevel = 0;
                expUpResPacket.currentExp = 0;
                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
            }
        }
}


//  ---------------------------- INVENTORY  ----------------------------

void RedisManager::AddItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto addItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
    
    ADD_ITEM_RESPONSE addItemResPacket;
    addItemResPacket.PacketId = (uint16_t)PACKET_ID::ADD_ITEM_RESPONSE;
    addItemResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);

    std::string inventory_slot = itemType[addItemReqPacket->itemType] + ":";
    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";

        if (redis->hset(inventory_slot, std::to_string(addItemReqPacket->itemSlotPos), std::to_string(addItemReqPacket->itemCode) +","+std::to_string(addItemReqPacket->itemCount))) { // AddItem Success (ItemCode:slotposition, count)
            addItemResPacket.isSuccess = true;
        }
        else { // AddItem Fail
            addItemResPacket.isSuccess = false;
        }


    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(ADD_ITEM_RESPONSE),(char*)&addItemResPacket);
}

void RedisManager::DeleteItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delItemReqPacket = reinterpret_cast<DEL_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    DEL_ITEM_RESPONSE delItemResPacket;
    delItemResPacket.PacketId = (uint16_t)PACKET_ID::DEL_ITEM_RESPONSE;
    delItemResPacket.PacketLength = sizeof(DEL_ITEM_RESPONSE);

    std::string inventory_slot = itemType[delItemReqPacket->itemType] + ":";
    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";

        if (redis->hdel(inventory_slot, std::to_string(delItemReqPacket->itemSlotPos))) { // DeleteItem Success
            delItemResPacket.isSuccess = true;
        }
        else { // DeleteItem Fail
            delItemResPacket.isSuccess = false;
        }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(DEL_ITEM_RESPONSE),(char*)&delItemResPacket);
}

void RedisManager::ModifyItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto modItemReqPacket = reinterpret_cast<MOD_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MOD_ITEM_RESPONSE modItemResPacket;
    modItemResPacket.PacketId = (uint16_t)PACKET_ID::MOD_ITEM_RESPONSE;
    modItemResPacket.PacketLength = sizeof(MOD_ITEM_RESPONSE);

    std::string inventory_slot = itemType[modItemReqPacket->itemType] + ":";
    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";

        if (redis->hset(inventory_slot, itemType[modItemReqPacket->itemType] + std::to_string(modItemReqPacket->itemCode) + std::to_string(modItemReqPacket->itemSlotPos), std::to_string(modItemReqPacket->itemCount))) { // ModifyItem Success
            modItemResPacket.isSuccess = true;
        }
        else { // ModifyItem Fail
            modItemResPacket.isSuccess = false;
        }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOD_ITEM_RESPONSE), (char*)&modItemResPacket);
}

void RedisManager::MoveItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto movItemReqPacket = reinterpret_cast<MOV_ITEM_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

        std::string tag = "{" + std::to_string(tempUser->GetPk()) +"}";
        std::string inventory_slot = itemType[movItemReqPacket->ItemType] + ":" + tag;

        auto pipe = redis->pipeline(tag);

        pipe.hset(inventory_slot, std::to_string(movItemReqPacket->dragItemSlotPos), std::to_string(movItemReqPacket->dragItemCode) + "," + std::to_string(movItemReqPacket->dragItemCount))
            .hset(inventory_slot, std::to_string(movItemReqPacket->targetItemSlotPos), std::to_string(movItemReqPacket->targetItemCode) + "," + std::to_string(movItemReqPacket->targetItemCount));

        pipe.exec();

        MOV_ITEM_RESPONSE movItemResPacket;
        movItemResPacket.PacketId = (uint16_t)PACKET_ID::MOV_ITEM_RESPONSE;
        movItemResPacket.PacketLength = sizeof(MOV_ITEM_RESPONSE);

        
        movItemResPacket.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOV_ITEM_RESPONSE), (char*)&movItemResPacket);
}


//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

void RedisManager::AddEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto addEquipReqPacket = reinterpret_cast<ADD_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    ADD_EQUIPMENT_RESPONSE addEquipResPacket;
    addEquipResPacket.PacketId = (uint16_t)PACKET_ID::ADD_EQUIPMENT_RESPONSE;
    addEquipResPacket.PacketLength = sizeof(ADD_EQUIPMENT_RESPONSE);

        std::string inventory_slot = "inventory:" + tempUser->GetPk();

        if (redis->hset(inventory_slot, std::to_string(addEquipReqPacket->itemSlotPos),std::to_string(addEquipReqPacket->itemCode) +"," + std::to_string(addEquipReqPacket->currentEnhanceCount))) { // AddItem Success (ItemCode:slotposition, count)
            addEquipResPacket.isSuccess = true;
        }
        else { // AddItem Fail
            addEquipResPacket.isSuccess = false;
        }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(ADD_EQUIPMENT_RESPONSE), (char*)&addEquipResPacket);
}

void RedisManager::DeleteEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<DEL_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    DEL_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (uint16_t)PACKET_ID::DEL_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(DEL_EQUIPMENT_RESPONSE);

    std::string inventory_slot = "inventory:" + tempUser->GetPk();

        if (redis->hdel(inventory_slot, std::to_string(delEquipReqPacket->itemSlotPos))) { // DeleteItem Success
            delEquipResPacket.isSuccess = true;
        }
        else { // DeleteItem Fail
            delEquipResPacket.isSuccess = false;
        }

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}

void RedisManager::EnhanceEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<ENH_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    ENH_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (uint16_t)PACKET_ID::ENH_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(ENH_EQUIPMENT_RESPONSE);

    std::string inventory_slot = "inventory:" + tempUser->GetPk();

        if (1) { // ���� ��ȭ�ϴ� hset or hincryby�� ����
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

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}

void RedisManager::MoveEquipment(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto movItemReqPacket = reinterpret_cast<MOV_EQUIPMENT_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    std::string tag = "{" + std::to_string(tempUser->GetPk()) + "}";
    std::string inventory_slot = itemType[0] + ":" + tag;

    auto pipe = redis->pipeline(tag);

    pipe.hset(inventory_slot, std::to_string(movItemReqPacket->dragItemSlotPos), std::to_string(movItemReqPacket->dragItemCode) + "," + std::to_string(movItemReqPacket->dragItemEnhance))
        .hset(inventory_slot, std::to_string(movItemReqPacket->targetItemSlotPos), std::to_string(movItemReqPacket->targetItemCode) + "," + std::to_string(movItemReqPacket->targetItemEnhance));

    pipe.exec();

    MOV_EQUIPMENT_RESPONSE movItemResPacket;
    movItemResPacket.PacketId = (uint16_t)PACKET_ID::MOV_EQUIPMENT_RESPONSE;
    movItemResPacket.PacketLength = sizeof(MOV_EQUIPMENT_RESPONSE);


    movItemResPacket.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOV_ITEM_RESPONSE), (char*)&movItemResPacket);
}


//  ---------------------------- RAID  ----------------------------

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { 
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    RAID_MATCHING_RESPONSE raidMatchResPacket;
    raidMatchResPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    raidMatchResPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);

    if (matchingManager->Insert(connObjNum_, tempUser)) { // Insert Into Mathcing Queue Success
        raidMatchResPacket.insertSuccess = true;
    }
    else raidMatchResPacket.insertSuccess = false; // Matching Queue Full

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&raidMatchResPacket);
}

void RedisManager::RaidReqTeamInfo(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidTeamInfoReqPacket = reinterpret_cast<RAID_TEAMINFO_REQUEST*>(pPacket_);

    if (!raidTeamInfoReqPacket->imReady) { // ���̵� �غ� ���� => �� ����
        return;
    }

    Room* tempRoom = roomManager->GetRoom(raidTeamInfoReqPacket->roomNum);
    tempRoom->setSockAddr(raidTeamInfoReqPacket->myNum, raidTeamInfoReqPacket->userAddr); // Set User UDP Socket Info

    InGameUser* teamUser = tempRoom->GetTeamUser(raidTeamInfoReqPacket->myNum);

    RAID_TEAMINFO_RESPONSE raidTeamInfoResPacket;
    raidTeamInfoResPacket.PacketId = (uint16_t)PACKET_ID::RAID_TEAMINFO_RESPONSE;
    raidTeamInfoResPacket.PacketLength = sizeof(RAID_TEAMINFO_RESPONSE);
    raidTeamInfoResPacket.teamLevel = teamUser->GetLevel();
    strncpy_s(raidTeamInfoResPacket.teamId, teamUser->GetId().c_str(), MAX_USER_ID_LEN);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_TEAMINFO_RESPONSE), (char*)&raidTeamInfoResPacket);

    if (tempRoom->StartCheck()) { // �� ���� �������� ���� ������ �����ϰ� �� �� ���� Ȯ���ϸ� ���� ���� ���� �����ֱ�
        RAID_START_REQUEST raidStartReqPacket1;
        raidStartReqPacket1.PacketId = (uint16_t)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket1.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket1.endTime = tempRoom->SetEndTime();

        RAID_START_REQUEST raidStartReqPacket2;
        raidStartReqPacket2.PacketId = (uint16_t)PACKET_ID::RAID_START_REQUEST;
        raidStartReqPacket2.PacketLength = sizeof(RAID_START_REQUEST);
        raidStartReqPacket2.endTime = tempRoom->SetEndTime();

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket1);
        connUsersManager->FindUser(tempRoom->GetTeamObjNum(raidTeamInfoReqPacket->myNum))->PushSendMsg(sizeof(RAID_START_REQUEST), (char*)&raidStartReqPacket2);
    }
}

void RedisManager::RaidHit(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto raidHitReqPacket = reinterpret_cast<RAID_HIT_REQUEST*>(pPacket_);
    InGameUser* user = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    RAID_HIT_RESPONSE raidHitResPacket;
    raidHitResPacket.PacketId = (uint16_t)PACKET_ID::RAID_HIT_RESPONSE;
    raidHitResPacket.PacketLength = sizeof(RAID_HIT_RESPONSE);

    auto room = roomManager->GetRoom(raidHitReqPacket->roomNum);
    auto hit = room->Hit(raidHitReqPacket->myNum, raidHitReqPacket->damage);

    if (hit.first <= 0) { // Mob Dead
        if (room->EndCheck()) { // SendEndMsg
            raidHitResPacket.currentMobHp = 0;
            raidHitResPacket.yourScore = hit.second;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);

            InGameUser* inGameUser;
            auto pipe = redis->pipeline("ranking");

            for (int i = 0; i < room->GetRoomUserCnt(); i++) {  // ���̵� ���� �޽���
                inGameUser = room->GetUser(i);

                RAID_END_REQUEST raidEndReqPacket;
                raidEndReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_END_REQUEST;
                raidEndReqPacket.PacketLength = sizeof(RAID_END_REQUEST);
                raidEndReqPacket.userScore = room->GetScore(i);
                raidEndReqPacket.teamScore = room->GetTeamScore(i);
                connUsersManager->FindUser(room->GetUserObjNum(i))->PushSendMsg(sizeof(RAID_END_REQUEST), (char*)&raidEndReqPacket);

                if (room->GetScore(i) > room->GetUser(i)->GetScore()) {
                    pipe.zadd("ranking", inGameUser->GetId(), (double)(room->GetScore(i))); // ���� ���𽺿� ����ȭ
                }
            }

            pipe.exec(); // ������ ��ŷ ����ȭ

            matchingManager->DeleteMob(room); // �� ���� ó��
        }
        else { // if get 0, waitting End message
            raidHitResPacket.currentMobHp = 0;
            raidHitResPacket.yourScore = hit.second;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
        }
    }
    else {
        raidHitResPacket.currentMobHp = hit.first;
        raidHitResPacket.yourScore = hit.second;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
    }

    if (hit.second != 0) { // Score�� 0�� �ƴϸ� �� ������ ����ȭ �޽��� ���� 
        connUsersManager->FindUser(webServerObjNum)->PushSendMsg(sizeof(RAID_HIT_RESPONSE), (char*)&raidHitResPacket);
    }
}

void RedisManager::GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<RAID_RANKING_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
   
    std::vector<std::pair<std::string, double>> scores;
    redis->zrevrange("ranking", delEquipReqPacket->startRank, delEquipReqPacket->startRank+ RANKING_USER_COUNT, std::back_inserter(scores));

    char* tempC = new char[MAX_SCORE_SIZE + 1];
    char* tc = tempC;
    uint16_t cnt = scores.size();

    for (int i = 0; i < cnt; i++) {
        RANKING ranking;
        strncpy_s(ranking.userId, scores[i].first.c_str(), MAX_USER_ID_LEN);
        ranking.score = scores[i].second;
        memcpy(tc, (char*)&ranking, sizeof(RANKING));
        tc += sizeof(RANKING);
    }

    RAID_RANKING_RESPONSE raidRankResPacket;
    raidRankResPacket.PacketId = (uint16_t)PACKET_ID::RAID_RANKING_RESPONSE;
    raidRankResPacket.PacketLength = sizeof(RAID_RANKING_RESPONSE);
    raidRankResPacket.rkCount = cnt;
    std::memcpy(raidRankResPacket.reqScore, tempC, MAX_SCORE_SIZE + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);
}