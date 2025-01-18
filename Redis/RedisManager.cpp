#include "RedisManager.h"

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

bool RedisManager::CreateRedisThread(const UINT16 RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisPool.emplace_back(std::thread([this]() {RedisThread(); }));
    }
};

void RedisManager::RedisThread() {
    while (redisRun) {
        std::unique_lock<std::mutex> lock(redisMu);

    }
};

void RedisManager::PushRedisPacket() {
    
};

void RedisManager::CloseMySQL() {
    
    mysql_close(ConnPtr);
}