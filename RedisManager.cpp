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

    // ---------- SET ITEM TYPE ---------- 
    itemType[1] = "equipment";
    itemType[2] = "consumables ";
    itemType[3] = "materials";

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
    inGameUserManager->Set(TempConnUser->GetObjNum(), userConn->userPk, userConn->level, userConn->currentExp);

    redis.persist("user:" + userConn->uuId); // Remove TTL Time
}

void RedisManager::Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { // Normal Disconnect
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    // Get Update Data From Redis 
    //"{user_info_" + TempConnUser->GetObjNumString() + "}"
    

    // Update Inven Data From Redis
    std::vector<std::string> keys = { TempConnUser->GetUuid() + ":equipment", TempConnUser->GetUuid() + ":consumable", TempConnUser->GetUuid() + ":material" };
    redis.hmget("{inventory_info}_" + TempConnUser->GetObjNumString(), keys.begin(), keys.end(), keys);

    std::string userInfoQuery = "update Users set last_login = current_timestamp, level = " + userInfoMap["level"] + "exp = " + userInfoMap["exp"] + "where id = " + userInfoMap["pk"];
    std::string invenQuery = "update inventory set ";

    const char* Query = &*userInfoQuery.begin();
    MysqlResult = mysql_query(ConnPtr, Query);

    if (MysqlResult != 0) {
        std::cerr << "MySQL UPDATE UserInfo Query Error: " << mysql_error(ConnPtr) << std::endl;
        mysql_query(ConnPtr, "ROLLBACK;"); // 무결성 오류 방지 ROLLBACK
        return;
    }

    Query = &*invenQuery.begin();
    MysqlResult = mysql_query(ConnPtr, Query);

    if (MysqlResult != 0) {
        std::cerr << "MySQL UPDATE UserInfo Query Error: " << mysql_error(ConnPtr) << std::endl;
        mysql_query(ConnPtr, "ROLLBACK;"); // 무결성 오류 방지 ROLLBACK
        return;
    }

    mysql_query(ConnPtr, "COMMIT;");

    TempConnUser->Reset(); // Initializes the ConnUser object

    redis.expire("user:"+ TempConnUser->GetUuid(), 180); // Set Short Time TTL (3 minutes)
    return;
}

void RedisManager::UserDisConnect(SOCKET userSkt) { // Abnormal Disconnect
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    // Get Update Data From Redis 
    std::unordered_map<std::string, std::string> userInfoMap;
    std::unordered_map<std::string, std::string> invenMap;
    redis.hgetall("user:" + TempConnUser->GetUuid(), std::inserter(userInfoMap, userInfoMap.begin()));
    redis.hgetall("inventory:" + TempConnUser->GetUuid(), std::inserter(invenMap, invenMap.begin()));

    // Update UserInfo In Mysql
    std::string userInfoQuery = "update Users set last_login = current_timestamp, level = " + userInfoMap["level"] + "exp = " + userInfoMap["exp"] + "where id = " + userInfoMap["pk"];

    const char* Query = &*userInfoQuery.begin();
    MysqlResult = mysql_query(ConnPtr, Query);

    if (MysqlResult != 0) {
        std::cerr << "MySQL UPDATE UserInfo Query Error: " << mysql_error(ConnPtr) << std::endl;
        mysql_query(ConnPtr, "ROLLBACK;"); // 무결성 오류 방지 ROLLBACK
        return;
    }

    // Update Inven Data From Redis
    std::string invenQuery = "update Users set last_login = current_timestamp, level = " + userInfoMap["level"] + "exp = " + userInfoMap["exp"] + "where id = " + userInfoMap["pk"];

    const char* Query = &*invenQuery.begin();
    MysqlResult = mysql_query(ConnPtr, Query);

    if (MysqlResult != 0) {
        std::cerr << "MySQL UPDATE UserInfo Query Error: " << mysql_error(ConnPtr) << std::endl;
        mysql_query(ConnPtr, "ROLLBACK;"); // 무결성 오류 방지 ROLLBACK
        return;
    }

    mysql_query(ConnPtr, "COMMIT;");

    TempConnUser->Reset(); // Initializes the ConnUser object

    redis.expire("user:" + TempConnUser->GetUuid(), 600); // Set Long Time TTL (10 minutes)
}

void RedisManager::ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    // Process Remain Packet

}


//  ---------------------------- USER_STATUS  ----------------------------

void RedisManager::ExpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto expUpReqPacket = reinterpret_cast<EXP_UP_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);

    EXP_UP_RESPONSE expUpResPacket;
    expUpResPacket.PacketId = (UINT16)PACKET_ID::ADD_ITEM_RESPONSE;
    expUpResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);
    expUpResPacket.uuId = TempConnUser->GetUuid();

    std::string user_slot = "userinfo:" + TempConnUser->GetUuid();

    std::pair<uint8_t, unsigned int> tempExp = inGameUserManager->ExpUp(TempConnUser->GetObjNum(), expUpReqPacket->increaseExp);

    if (tempExp.first == 0) {
        redis.hincrby(user_slot,"exp", tempExp.second);

        expUpResPacket.currentExp = tempExp.second;
        TempConnUser->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
    }
    else { // Level Up
        redis.hincrby(user_slot, "level", tempExp.first);
        redis.set(user_slot, "exp", tempExp.second);

        LEVEL_UP_RESPONSE levelUpResPacket;
        levelUpResPacket.PacketId = (UINT16)PACKET_ID::ADD_ITEM_RESPONSE;
        levelUpResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);
        levelUpResPacket.uuId = TempConnUser->GetUuid();

        expUpResPacket.currentExp = tempExp.second;
        levelUpResPacket.currentLevel = tempExp.first;
        TempConnUser->PushSendMsg(sizeof(EXP_UP_RESPONSE), (char*)&expUpResPacket);
        TempConnUser->PushSendMsg(sizeof(LEVEL_UP_RESPONSE), (char*)&levelUpResPacket);
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
    addEquipResPacket.PacketId = (UINT16)PACKET_ID::ADD_ITEM_RESPONSE;
    addEquipResPacket.PacketLength = sizeof(ADD_ITEM_RESPONSE);
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
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::DEL_ITEM_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(DEL_ITEM_RESPONSE);
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
    delEquipResPacket.PacketId = (UINT16)PACKET_ID::DEL_ITEM_RESPONSE;
    delEquipResPacket.PacketLength = sizeof(DEL_ITEM_RESPONSE);
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