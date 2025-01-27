#include "RedisManager.h"

void RedisManager::init(const UINT16 RedisThreadCnt_) {
    packetIDTable = std::vector<RECV_PACKET_FUNCTION>(PACKET_ID_SIZE, nullptr);

    //SYSTEM
    packetIDTable[1] = &RedisManager::Login;
    packetIDTable[2] = &RedisManager::Logout;
    packetIDTable[3] = &RedisManager::UserDisConnect;
    packetIDTable[4] = &RedisManager::ServerEnd;

    // USER STATUS
    packetIDTable[11] = &RedisManager::LevelUp;
    packetIDTable[12] = &RedisManager::LevelDown;
    packetIDTable[13] = &RedisManager::Exp_Up;
    packetIDTable[14] = &RedisManager::Exp_Down;
    packetIDTable[15] = &RedisManager::HpUp;
    packetIDTable[16] = &RedisManager::HpDown;
    packetIDTable[17] = &RedisManager::MpUp;
    packetIDTable[18] = &RedisManager::MpDown;

    // INVENTORY
    packetIDTable[25] = &RedisManager::AddItem;
    packetIDTable[26] = &RedisManager::DelItem;
    packetIDTable[27] = &RedisManager::MoveItem;

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

// ------------------------------------------------------------------------------

//SYSTEM
void RedisManager::Login(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::Logout(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::UserDisConnect(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::ServerEnd(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

// USER STATUS
void RedisManager::LevelUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::LevelDown(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}
void RedisManager::Exp_Up(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::Exp_Down(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::HpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::HpDown(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::MpUp(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::MpDown(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

// INVENTORY
void RedisManager::AddItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::DelItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}

void RedisManager::MoveItem(SOCKET userSkt, UINT16 packetSize_, char* pPacket_) {

}