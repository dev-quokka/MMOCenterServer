#include "RedisManager.h"

void RedisManager::init(const UINT16 RedisThreadCnt_) {

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
    ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "Quokka", 3306, (char*)NULL, 0);

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

// ---------------------------- PACKET -----------------------------------

// SYSTEM
void RedisManager::UserConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto uuidCheck = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(uuidCheck->uuId);

    redis.persist("user:" + uuidCheck->uuId); // Remove TTL Time
}

void RedisManager::Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) { // Normal Disconnect
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

    redis.expire("user:"+ TempConnUser->GetUuid(), 180); // Set TTL (Short Time)
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

    redis.expire("user:" + TempConnUser->GetUuid(), 600); // Set TTL (Long Time)
}

void RedisManager::ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

// USER_STATUS


// INVENTORY
void RedisManager::AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto uuidCheck = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(uuidCheck->uuId);

    std::string inventory_key = "inventory:uuid123:"+ itemType[uuidCheck->itemType];

    if (redis.hset(inventory_key, "101:0", "10")) { // AddItem Success (ItemCode:slotlocation, count)

    }
    else { // AddItem Fail

    }

}

void RedisManager::DeleteItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto uuidCheck = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(uuidCheck->uuId);

    std::string inventory_key = "inventory:uuid123:" + itemType[uuidCheck->itemType];

    if (redis.hdel(inventory_key, "101:0")) { // DeleteItem Success
    
    }
    else { // DeleteItem Fail

    }
}

void RedisManager::MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto uuidCheck = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(uuidCheck->uuId);

    std::string inventory_key = "inventory:uuid123:" + itemType[uuidCheck->itemType];

    if (redis.hset(inventory_key, "101:0", "20")) { // MoveItem Success
    
    }
    else { // MoveItem Fail

    }
}

void RedisManager::ModifyItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {
    auto uuidCheck = reinterpret_cast<ADD_ITEM_REQUEST*>(pPacket_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt);
    TempConnUser->SetUuid(uuidCheck->uuId);

    std::string inventory_key = "inventory:uuid123:" + itemType[uuidCheck->itemType];

    if (redis.hset(inventory_key, "101:0", "20")) { // ModifyItem Success

    }
    else {// ModifyItem Fail

    }
}