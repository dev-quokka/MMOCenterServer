#include "RedisManager.h"

void RedisManager::RedisRun(const UINT16 RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        // Redis Ŭ������ ����
        redis = sw::redis::RedisCluster(connection_options);
        std::cout << "Redis Ŭ������ ���� ����!" << std::endl;

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis ���� �߻�: " << err.what() << std::endl;
    }
}

void RedisManager::MysqlRun() {
    mysql_init(&Conn);
    ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "Quokka", 3306, (char*)NULL, 0);

    if (ConnPtr == NULL) std::cout << "MySQL Connect Fail" << std::endl; // mysql ���� ����
    else std::cout << "MySQL Connect Success" << std::endl; // mysql ���� ����
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
};

void RedisManager::SendMsg(SOCKET tempSkt_) { // Send Proccess Message To User
    ConnUser* TempConnUser = connUsersManager->FindUser(tempSkt_);        
    //TempConnUser->PushSendMsg();
};

void RedisManager::RedisThread() {
    DataPacket tempD(0,0);
    ConnUser* TempConnUser = nullptr;
    while (redisRun) {
        if (procSktQueue.pop(tempD)) {
            
        }
    }
};

void RedisManager::PushRedisPacket(const SOCKET userSkt_, const UINT32 size_, char* recvData_) {
    DataPacket tempD(size_,userSkt_);
    ConnUser* TempConnUser = connUsersManager->FindUser(userSkt_);
    TempConnUser->WriteRecvData(recvData_,size_); // Push Data in Circualr Buffer
    procSktQueue.push(tempD);
};

void RedisManager::CloseMySQL() {
    
    mysql_close(ConnPtr);
}