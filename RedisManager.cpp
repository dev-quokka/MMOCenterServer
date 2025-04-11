#include "RedisManager.h"

thread_local std::mt19937 RedisManager::gen(std::random_device{}());

void RedisManager::init(const uint16_t RedisThreadCnt_) {

    ServerAddressMap[ServerType::GatewayServer] = { "127.0.0.1", 9091 };
    ServerAddressMap[ServerType::MatchingServer] = { "127.0.0.1", 9131 };
    ServerAddressMap[ServerType::ChannelServer01] = { "127.0.0.1", 9211 };
    ServerAddressMap[ServerType::ChannelServer02] = { "127.0.0.1", 9221 };
    ServerAddressMap[ServerType::RaidGameServer01] = { "127.0.0.1", 9501 };

    // ---------- SET PACKET PROCESS ---------- 
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    // SYSTEM
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::USER_LOGOUT_REQUEST] = &RedisManager::Logout;
    packetIDTable[(uint16_t)PACKET_ID::SERVER_USER_COUNTS_REQUEST] = &RedisManager::SendServerUserCounts;
    packetIDTable[(uint16_t)PACKET_ID::MOVE_SERVER_REQUEST] = &RedisManager::MoveServer;
    
    // SESSION
    packetIDTable[(uint16_t)PACKET_ID::IM_SESSION_REQUEST] = &RedisManager::ImSessionRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::IM_CHANNEL_REQUEST] = &RedisManager::ImChannelRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;

    // MATCHING
    packetIDTable[(uint16_t)PACKET_ID::IM_MATCHING_REQUEST] = &RedisManager::ImMatchingRequest;

    // RAID
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchStartResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchingCancelResponse;
    packetIDTable[(uint16_t)PACKET_ID::RAID_RANKING_REQUEST] = &RedisManager::GetRanking;

    channelServerObjNums.resize(3, 0); // ������ ���� �� + 1
    raidGameServerObjNums.resize(2, 0); // ������ ���� �� + 1

    RedisRun(RedisThreadCnt_);
    channelServersManager = new ChannelServersManager;
    channelServersManager->init();
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

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_) {
    connUsersManager = connUsersManager_;
    inGameUserManager = inGameUserManager_;
}

bool RedisManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    for (int i = 0; i < RedisThreadCnt_; i++) {
        redisThreads.emplace_back(std::thread([this]() {RedisThread(); }));
    }
    return true;
}

bool RedisManager::EquipmentEnhance(uint16_t currentEnhanceCount_) {
    if (currentEnhanceCount_ < 0 || currentEnhanceCount_ >= enhanceProbabilities.size()) {
        return false;
    }

    std::uniform_int_distribution<int> dist(1, 100);
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

    USER_CONNECT_RESPONSE_PACKET ucReq;
    ucReq.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_RESPONSE;
    ucReq.PacketLength = sizeof(USER_CONNECT_RESPONSE_PACKET);
    try {
        auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));
        if (pk) {
            std::string userInfokey = "userinfo:{" + std::to_string(pk) + "}";
            std::unordered_map<std::string, std::string> userData;
            redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

            connUsersManager->FindUser(connObjNum_)->SetPk(pk);
            inGameUserManager->Set(connObjNum_, (std::string)userConn->userId, pk, std::stoul(userData["exp"]),
            static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]));

            ucReq.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " Connect" << std::endl;
        }
        else {
            ucReq.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
            std::cout << (std::string)userConn->userId << " JWT Check Fail" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucReq.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void RedisManager::Logout(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { // Normal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
        std::cout << "���� �α׾ƿ� ��ũ�� �޽��� ����" << std::endl;
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) { // Abnormal Disconnect
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    {  // Send User PK to the Session Server for Synchronization with MySQL
        SYNCRONIZE_LOGOUT_REQUEST syncLogoutReqPacket;
        syncLogoutReqPacket.PacketId = (uint16_t)PACKET_ID::SYNCRONIZE_LOGOUT_REQUEST;
        syncLogoutReqPacket.PacketLength = sizeof(SYNCRONIZE_LOGOUT_REQUEST);
        syncLogoutReqPacket.userPk = tempUser->GetPk();
        connUsersManager->FindUser(GatewayServerObjNum)->PushSendMsg(sizeof(SYNCRONIZE_LOGOUT_REQUEST), (char*)&syncLogoutReqPacket);
        std::cout << "���� ��Ŀ��Ʈ ��ũ�� �޽��� ����" << std::endl;
    }
}

void RedisManager::ImSessionRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<IM_SESSION_REQUEST*>(pPacket_);
    std::cout << "Session Server Connect Request : " << connObjNum_ << std::endl;

    IM_SESSION_RESPONSE imSessionResPacket;
    imSessionResPacket.PacketId = (uint16_t)PACKET_ID::IM_SESSION_RESPONSE;
    imSessionResPacket.PacketLength = sizeof(IM_SESSION_RESPONSE);

    std::string str(userConn->Token);

    try {
        auto pk = redis->hget("jwtcheck:{sessionserver}", str);

        if (pk) {
            connUsersManager->FindUser(connObjNum_)->SetPk(static_cast<uint32_t>(std::stoul(*pk)));
            GatewayServerObjNum = connObjNum_;
            imSessionResPacket.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
            std::cout << "Session Server Connect Success : " << connObjNum_ << std::endl;
        }
        else {
            imSessionResPacket.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
            return;
        }
    }
    catch (const sw::redis::Error& e) {
        imSessionResPacket.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_SESSION_RESPONSE), (char*)&imSessionResPacket);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}

void RedisManager::ImChannelRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imChReqPacket = reinterpret_cast<IM_CHANNEL_REQUEST*>(pPacket_);
    channelServerObjNums[imChReqPacket->channelServerNum] = connObjNum_; // ä�� ���� ������ȣ ����
    std::cout << "Channel Server" << imChReqPacket->channelServerNum <<" Connect Request : " << connObjNum_ << std::endl;

    IM_CHANNEL_RESPONSE imChRes;
    imChRes.PacketId = (uint16_t)PACKET_ID::IM_CHANNEL_RESPONSE;
    imChRes.PacketLength = sizeof(IM_CHANNEL_RESPONSE);
    imChRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&imChRes);
    std::cout << "Channel Server" << imChReqPacket->channelServerNum << " Connect Success : " << connObjNum_ << std::endl;
}

void RedisManager::ImMatchingRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<IM_MATCHING_REQUEST*>(pPacket_);
    MatchingServerObjNum = connObjNum_;

    std::cout << "Channel Server Connect Request : " << connObjNum_ << std::endl;

    IM_MATCHING_RESPONSE imMRes;
    imMRes.PacketId = (uint16_t)PACKET_ID::IM_MATCHING_RESPONSE;
    imMRes.PacketLength = sizeof(IM_MATCHING_RESPONSE);
    imMRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(IM_MATCHING_RESPONSE), (char*)&imMRes);
    std::cout << "Channel Server Connect Success : " << connObjNum_ << std::endl;
}

void RedisManager::SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
	SERVER_USER_COUNTS_RESPONSE serverUserCountsResPacket;
    serverUserCountsResPacket.PacketId = (uint16_t)PACKET_ID::SERVER_USER_COUNTS_RESPONSE;
    serverUserCountsResPacket.PacketLength = sizeof(SERVER_USER_COUNTS_RESPONSE);
    auto tempV = channelServersManager->GetServerCounts();

    char* tempC = new char[MAX_SERVER_USERS + 1];
    char* tc = tempC;
    uint16_t cnt = tempV.size();

    for (int i = 1; i < cnt; i++) {
		uint16_t userCount = tempV[i];
        memcpy(tc, (char*)&userCount, sizeof(uint16_t));
        tc += sizeof(uint16_t);
    }

    serverUserCountsResPacket.serverCount = cnt;
    std::memcpy(serverUserCountsResPacket.serverUserCnt, tempC, MAX_SERVER_USERS + 1);

    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&serverUserCountsResPacket);

    delete[] tempC;
}

void RedisManager::ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<USER_DISCONNECT_AT_CHANNEL_REQUEST*>(pPacket_);
    channelServersManager->LeaveChannelServer(MoveCHReqPacket->channelServerNum);
}

void RedisManager::MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<MOVE_SERVER_REQUEST*>(pPacket_);
    MOVE_SERVER_RESPONSE moveCHResPacket;
    std::string tag;

    if (MoveCHReqPacket->serverNum == 1) {
        moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
        moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
		moveCHResPacket.port = ServerAddressMap[ServerType::ChannelServer01].port;
        strncpy_s(moveCHResPacket.ip, ServerAddressMap[ServerType::ChannelServer01].ip.c_str(), 256);
        tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::ChannelServer01)) + "}";

        if (!channelServersManager->EnterChannelServer(static_cast<uint16_t>(ChannelServerType::CH_01))) {// �ο��� �̸� �Ѹ� ���� (���н� ���� ó��)
            moveCHResPacket.port = 0;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        };

	}
	else if (MoveCHReqPacket->serverNum == 2) {
        moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
        moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
        moveCHResPacket.port = ServerAddressMap[ServerType::ChannelServer02].port;
        strncpy_s(moveCHResPacket.ip, ServerAddressMap[ServerType::ChannelServer02].ip.c_str(), 256);
        tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::ChannelServer02)) + "}";

        if (!channelServersManager->EnterChannelServer(static_cast<uint16_t>(ChannelServerType::CH_02))) {// �ο��� �̸� �Ѹ� ���� (���н� ���� ó��)
            moveCHResPacket.port = 0;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        };
	}

    // ä�� �̵��� ������ ���� JWT Token ����
    std::string token = jwt::create()
        .set_issuer("Center_Server")
        .set_subject("Move_Server")
        .set_payload_claim("user_id", jwt::claim(inGameUserManager->GetInGameUserByObjNum(connObjNum_)->GetId()))
        .set_expires_at(std::chrono::system_clock::now() +
            std::chrono::seconds{ 300 })
        .sign(jwt::algorithm::hs256{ JWT_SECRET });

    std::string key = "jwtcheck:" + tag;

    auto pipe = redis->pipeline(tag);

    pipe.hset(key, token, std::to_string(inGameUserManager->GetInGameUserByObjNum(connObjNum_)->GetPk())) // ���� Ŭ�����Ϳ� �ش� Token�� key�� ���� PK ����
        .expire(key, 300);

    pipe.exec();

    strncpy_s(moveCHResPacket.serverToken, token.c_str(), 256);
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket); // �������� �̵��� ä�� ������ JWT Token ����
}


//  ---------------------------- RAID  ----------------------------

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { 
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_REQUEST_TO_MATCHING_SERVER matchReqPacket;
    matchReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER;
    matchReqPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER);
    matchReqPacket.userPk = tempUser->GetPk();
	matchReqPacket.userCenterObjNum = connObjNum_;
    matchReqPacket.userGroupNum = tempUser->GetLevel()/3 + 1; // �����ص� �׷� ��ȣ ���� ����

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER), (char*)&matchReqPacket);
}

void RedisManager::MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    if (matchSuccessReqPacket->isSuccess) {
        std::cout << tempUser->GetId() << " " << tempUser->GetLevel() / 3 + 1 << " �׷� Insert Success" << std::endl;
    }

    RAID_MATCHING_RESPONSE matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    matchResPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);
    matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;

    connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);
    std::cout << tempUser->GetId() << " " << tempUser->GetLevel() / 3 + 1 << " �׷� Insert Fail" << std::endl;
}

void RedisManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER matchCancelReqPacket;
    matchCancelReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER;
    matchCancelReqPacket.PacketLength = sizeof(MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER);
    matchCancelReqPacket.userCenterObjNum = connObjNum_;
    matchCancelReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1;

    connUsersManager->FindUser(MatchingServerObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCancelReqPacket);
}

void RedisManager::MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchCancelResPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchCancelResPacket->userCenterObjNum);

    MATCHING_CANCEL_RESPONSE matchCanResPacket;
    matchCanResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE;
    matchCanResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE);
    matchCanResPacket.isSuccess = matchCancelResPacket->isSuccess;

    connUsersManager->FindUser(matchCancelResPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCanResPacket);
    std::cout << tempUser->GetId() << " Matching Cancel Success" << std::endl;
}

void RedisManager::MatchFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {

}

void RedisManager::MatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    return;
    
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_SUCCESS_RESPONSE_TO_CENTER_SERVER*>(pPacket_);

	uint16_t tempRoomNum = matchSuccessReqPacket->roomNum;

	RAID_READY_REQUEST raidReadyReqPacket;
	raidReadyReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
	raidReadyReqPacket.PacketLength = sizeof(RAID_READY_REQUEST);
    raidReadyReqPacket.roomNum = tempRoomNum;
    raidReadyReqPacket.udpPort = 50001; // ���߿� ���� ������ �þ�� �ش� ������ ���� udp ��Ʈ ���� �ޱ�
    raidReadyReqPacket.port = ServerAddressMap[ServerType::RaidGameServer01].port;
    strncpy_s(raidReadyReqPacket.ip, ServerAddressMap[ServerType::RaidGameServer01].ip.c_str(), 256);

    { // ��Ī�� �����鿡�� ���õ� ���� ������ ip, port�� ä�� �̵� �� ������ ���� JWT Token ���� (������ �������� vector �̿� ���)
        std::string token1 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userObjNum1)))  // ���� ������ȣ
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum)))  // �� ��ȣ
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        std::string tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);

        pipe.hset(key, token1, std::to_string(matchSuccessReqPacket->userObjNum1))
            .expire(key, 300);

        connUsersManager->FindUser(matchSuccessReqPacket->userObjNum1)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        std::string token2 = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Connect_GameServer")
            .set_payload_claim("user_id", jwt::claim(std::to_string(matchSuccessReqPacket->userObjNum2)))  // ���� ������ȣ
            .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum)))  // �� ��ȣ
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });

        pipe.hset(key, token2, std::to_string(matchSuccessReqPacket->userObjNum2))
            .expire(key, 150);

        connUsersManager->FindUser(matchSuccessReqPacket->userObjNum2)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

        pipe.exec();
    }
}

void RedisManager::MatchStartFail(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {

}

void RedisManager::GetRanking(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<RAID_RANKING_REQUEST*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    RAID_RANKING_RESPONSE raidRankResPacket;
    raidRankResPacket.PacketId = (uint16_t)PACKET_ID::RAID_RANKING_RESPONSE;
    raidRankResPacket.PacketLength = sizeof(RAID_RANKING_RESPONSE);

    std::vector<std::pair<std::string, double>> scores;
    try {

        redis->zrevrange("ranking", delEquipReqPacket->startRank, 
            delEquipReqPacket->startRank + RANKING_USER_COUNT, std::back_inserter(scores));

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

        raidRankResPacket.rkCount = cnt;
        std::memcpy(raidRankResPacket.reqScore, tempC, MAX_SCORE_SIZE + 1);

        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);

        delete[] tempC;
    }
    catch (const sw::redis::Error& e) {
        raidRankResPacket.rkCount = 0;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&raidRankResPacket);
        std::cerr << "Redis error: " << e.what() << std::endl;
        return;
    }
}