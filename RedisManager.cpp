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

    inGameUserManager->Init(maxClientCount_);

    RedisManager::RedisRun(RedisThreadCnt_);
    RedisManager::MysqlRun();
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

void RedisManager::MysqlRun() {
    mysql_init(&Conn);
    ConnPtr = mysql_real_connect(&Conn, MYSQL_HOST, MYSQL_USER, MYSQL_PASSWORD , MYSQL_DB , 3306, (char*)NULL, 0);

    if (ConnPtr == NULL) std::cout << "MySQL Connect Fail" << std::endl; // mysql 연결 실패
    else std::cout << "MySQL Connect Success" << std::endl; // mysql 연결 성공
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
        redisPool.emplace_back(std::thread([this]() {RedisThread(); }));
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

void RedisManager::SendMsg(SOCKET tempSkt_) { // Send Proccess Message To User
    ConnUser* TempConnUser = connUsersManager->FindUser(tempSkt_);        
    //TempConnUser->PushSendMsg();
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

void RedisManager::CloseMySQL() {
    
    mysql_close(ConnPtr);
}


// ---------------------------- PACKET ----------------------------

//  ---------------------------- SYSTEM  ----------------------------

void RedisManager::UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(userConn->uuId);
    TempConnUser->SetPk(userConn->userPk);
    inGameUserManager->Set(TempConnUser->GetObjNum(), userConn->level, userConn->currentExp);

    redis.persist("user:" + userConn->uuId); // Remove TTL Time
}

void RedisManager::Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { // Normal Disconnect
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    ConnUser* TempWebServer = connUsersManager->FindUser(webServerSocket);

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.uuId = TempConnUser->GetUuid();
        syncLogoutReqPacket.userPk = TempConnUser->GetPk();

        TempWebServer->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
    }
}

void RedisManager::UserDisConnect(SOCKET userSkt) { // Abnormal Disconnect
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    ConnUser* TempWebServer = connUsersManager->FindUser(webServerSocket);

    {  // Send User PK to the Web Server for Synchronization with MySQL
        SYNCRONIZE_DISCONNECT_REQUEST syncDisconnReqPacket;
        syncDisconnReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_DISCONNECT_REQUEST;
        syncDisconnReqPacket.PacketLength = sizeof(SYNCRONIZE_DISCONNECT_REQUEST);
        syncDisconnReqPacket.uuId = TempConnUser->GetPk();

        TempWebServer->PushSendMsg(sizeof(SYNCRONIZE_DISCONNECT_REQUEST), (char*)&syncDisconnReqPacket);
    }
}

void RedisManager::ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    // Process Remain Packet

}

void RedisManager::ImWebRequest(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    IM_WEB_RESPONSE imWebResPacket;
    imWebResPacket.PacketId = (UINT16)PACKET_ID::IM_WEB_RESPONSE;
    imWebResPacket.PacketLength = sizeof(IM_WEB_RESPONSE);
    imWebResPacket.uuId = TempConnUser->GetUuid();

    if (webServerSocket != 0) { // Web Server Already Exist
        imWebResPacket.isSuccess = false;
        TempConnUser->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
        return;
    }

    webServerSocket = userSkt;
    imWebResPacket.isSuccess = true;

    TempConnUser->PushSendMsg(sizeof(IM_WEB_RESPONSE), (char*)&imWebResPacket);
}


//  ---------------------------- USER_STATUS  ----------------------------

void RedisManager::ExpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto expUpReqPacket = reinterpret_cast<EXP_UP_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    EXP_UP_RESPONSE expUpResPacket;
    expUpResPacket.PacketId = (UINT16)PACKET_ID::EXP_UP_RESPONSE;
    expUpResPacket.PacketLength = sizeof(EXP_UP_RESPONSE);
    expUpResPacket.uuId = TempConnUser->GetUuid();

    std::string user_slot = "userinfo:" + TempConnUser->GetUuid();

    if (redis.hincrby(user_slot, "exp", mobExp[expUpReqPacket->mobNum])) { // Exp Up Success
        auto userExp = inGameUserManager->ExpUp(TempConnUser->GetObjNum(), mobExp[expUpReqPacket->mobNum]); // Increase Level Cnt , Current Exp

        if (userExp.first!=0) { // Level Up
            LEVEL_UP_RESPONSE levelUpResPacket;
            levelUpResPacket.PacketId = (UINT16)PACKET_ID::LEVEL_UP_RESPONSE;
            levelUpResPacket.PacketLength = sizeof(LEVEL_UP_RESPONSE);
            levelUpResPacket.uuId = TempConnUser->GetUuid();

            if (redis.hincrby(user_slot, "level", userExp.first)) { // Level Up Success
                levelUpResPacket.increaseLevel = userExp.first;
                levelUpResPacket.currentExp = userExp.second;

                TempConnUser->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&levelUpResPacket);

                { // Send User PK, Level, Exp data to the Web Server for Synchronization with MySQL
                    ConnUser* TempWebServer = connUsersManager->FindUser(webServerSocket);

                    SYNCRONIZE_LEVEL_REQUEST syncLevelReqPacket;
                    syncLevelReqPacket.PacketId = (UINT16)PACKET_ID::SYNCRONIZE_LEVEL_REQUEST;
                    syncLevelReqPacket.PacketLength = sizeof(SYNCRONIZE_LEVEL_REQUEST);
                    syncLevelReqPacket.uuId = TempWebServer->GetUuid();
                    syncLevelReqPacket.level = userExp.first;
                    syncLevelReqPacket.currentExp = userExp.second;
                    syncLevelReqPacket.userPk = TempConnUser->GetPk();

                    TempWebServer->PushSendMsg(sizeof(SYNCRONIZE_LEVEL_REQUEST), (char*)&syncLevelReqPacket);
                }
            }
            else { // Level Up Fail
                levelUpResPacket.increaseLevel = 0;
                levelUpResPacket.currentExp = 0;
                TempConnUser->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&levelUpResPacket);
            }
        }
        else { // Just Exp Up
            expUpResPacket.expUp = userExp.second;
            TempConnUser->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
        }
    }
    else{ // Exp Up Fail
        expUpResPacket.expUp = 0;
        TempConnUser->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
    }
}


//  ---------------------------- INVENTORY  ----------------------------

void RedisManager::AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto addItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    
    ADD_ITEM_RESPONSE addItemResPacket;
    addItemResPacket.PacketId = (UINT16)PACKET_ID::ADD_ITEM_RESPONSE;
    addItemResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);
    addItemResPacket.uuId = TempConnUser->GetUuid();

    if (addItemReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(ADD_ITEM_RESPONSE),(char*)&addItemResPacket);
}

void RedisManager::DeleteItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    DEL_ITEM_RESPONSE delItemResPacket;
    delItemResPacket.PacketId = (UINT16)PACKET_ID::DEL_ITEM_RESPONSE;
    delItemResPacket.PacketLength = sizeof(DEL_ITEM_RESPONSE);
    delItemResPacket.uuId = TempConnUser->GetUuid();

    if (delItemReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(DEL_ITEM_RESPONSE),(char*)&delItemResPacket);
}

void RedisManager::ModifyItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto modItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    MOD_ITEM_RESPONSE modItemResPacket;
    modItemResPacket.PacketId = (UINT16)PACKET_ID::MOD_ITEM_RESPONSE;
    modItemResPacket.PacketLength = sizeof(MOD_ITEM_RESPONSE);
    modItemResPacket.uuId = TempConnUser->GetUuid();

    if (modItemReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(MOD_ITEM_RESPONSE), (char*)&modItemResPacket);
}

void RedisManager::MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto movItemReqPacket = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    MOV_ITEM_RESPONSE movItemResPacket;
    movItemResPacket.PacketId = (UINT16)PACKET_ID::MOV_ITEM_RESPONSE;
    movItemResPacket.PacketLength = sizeof(MOV_ITEM_RESPONSE);
    movItemResPacket.uuId = TempConnUser->GetUuid();

    if (movItemReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(MOV_ITEM_RESPONSE), (char*)&movItemResPacket);
}


//  ---------------------------- INVENTORY:EQUIPMENT  ----------------------------

void RedisManager::AddEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto addEquipReqPacket = reinterpret_cast<ADD_EQUIPMENT_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    ADD_EQUIPMENT_RESPONSE addEquipResPacket;
    addEquipResPacket.PacketId = (UINT16)PACKET_ID::ADD_EQUIPMENT_RESPONSE;
    addEquipResPacket.PacketLength = sizeof(ADD_EQUIPMENT_RESPONSE);
    addEquipResPacket.uuId = TempConnUser->GetUuid();

    if (addEquipReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(ADD_EQUIPMENT_RESPONSE), (char*)&addEquipResPacket);
}

void RedisManager::DeleteEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<DEL_EQUIPMENT_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    DEL_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::DEL_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(DEL_EQUIPMENT_RESPONSE);
    delEquipResPacket.uuId = TempConnUser->GetUuid();

    if (delEquipReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}

void RedisManager::EnhanceEquipment(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<ENH_EQUIPMENT_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    ENH_EQUIPMENT_RESPONSE delEquipResPacket;
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::ENH_EQUIPMENT_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(ENH_EQUIPMENT_RESPONSE);
    delEquipResPacket.uuId = TempConnUser->GetUuid();

    if (delEquipReqPacket->uuId == TempConnUser->GetUuid()) { // UUID CORRECT
        std::string inventory_slot = "inventory:" + TempConnUser->GetUuid();

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

    TempConnUser->PushSendMsg(sizeof(DEL_EQUIPMENT_RESPONSE), (char*)&delEquipResPacket);
}