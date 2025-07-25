#include "RedisManager.h"

void RedisManager::Test_CashCahrge(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_){
    auto testCashCharge = reinterpret_cast<CASH_CHARGE_COMPLETE_REQUEST*>(pPacket_);
    ConnUser* user = connUsersManager->FindUser(connObjNum_);

    CASH_CHARGE_COMPLETE_RESPONSE ccRes;
    ccRes.PacketId = (uint16_t)PACKET_ID::CASH_CHARGE_COMPLETE_RESPONSE;
    ccRes.PacketLength = sizeof(CASH_CHARGE_COMPLETE_RESPONSE);

    std::string currencyTypeKey = "userinfo:{" + std::to_string(user->GetPk()) + "}";

    try {
        ccRes.chargedCash = redis->hincrby("userinfo:{" + std::to_string(user->GetPk()) + "}", "usercash", testCashCharge->chargedCash);
        ccRes.isSuccess = true;
        user->PushSendMsg(sizeof(ccRes), (char*)&ccRes);
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        user->PushSendMsg(sizeof(ccRes), (char*)&ccRes);
    }
}

// ========================== INITIALIZATION =========================

void RedisManager::init(const uint16_t RedisThreadCnt_) {

    // -------------------- SET PACKET HANDLERS ----------------------
    packetIDTable = std::unordered_map<uint16_t, RECV_PACKET_FUNCTION>();

    packetIDTable[(uint16_t)PACKET_ID::CASH_CHARGE_COMPLETE_REQUEST] = &RedisManager::Test_CashCahrge;

    // CENTER
    packetIDTable[(uint16_t)PACKET_ID::USER_CONNECT_REQUEST] = &RedisManager::UserConnect;
    packetIDTable[(uint16_t)PACKET_ID::SERVER_USER_COUNTS_REQUEST] = &RedisManager::SendServerUserCounts;
    packetIDTable[(uint16_t)PACKET_ID::MOVE_SERVER_REQUEST] = &RedisManager::MoveServer;
    packetIDTable[(uint16_t)PACKET_ID::SHOP_DATA_REQUEST] = &RedisManager::SendShopDataToClient;
    packetIDTable[(uint16_t)PACKET_ID::SHOP_BUY_ITEM_REQUEST] = &RedisManager::BuyItemFromShop;
    packetIDTable[(uint16_t)PACKET_ID::PASS_DATA_REQUEST] = &RedisManager::SendPassDataToClient;
    packetIDTable[(uint16_t)PACKET_ID::PASS_EXP_UP_REQUEST] = &RedisManager::PassExpUp;
    packetIDTable[(uint16_t)PACKET_ID::GET_PASS_ITEM_REQUEST] = &RedisManager::GetPassItem;

    // LOGIN
    packetIDTable[(uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_REQUEST] = &RedisManager::LoginServerConnectRequest;

    // CHANNEL
    packetIDTable[(uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_REQUEST] = &RedisManager::ChannelServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::USER_DISCONNECT_AT_CHANNEL_REQUEST] = &RedisManager::ChannelDisConnect;
    packetIDTable[(uint16_t)PACKET_ID::SYNC_EQUIPMENT_ENHANCE_REQUEST] = &RedisManager::SyncEqipmentEnhace;
    
    // MATCHING
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_REQUEST] = &RedisManager::MatchingServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::RAID_MATCHING_REQUEST] = &RedisManager::MatchStart;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchStartResponse;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST] = &RedisManager::MatchingCancel;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE_FROM_MATCHING_SERVER] = &RedisManager::MatchingCancelResponse;

    // RAID GAME
    packetIDTable[(uint16_t)PACKET_ID::RAID_SERVER_CONNECT_REQUEST] = &RedisManager::GameServerConnectRequest;
    packetIDTable[(uint16_t)PACKET_ID::MATCHING_RESPONSE_FROM_GAME_SERVER] = &RedisManager::CheckMatchSuccess;
    packetIDTable[(uint16_t)PACKET_ID::SYNC_HIGHSCORE_REQUEST] = &RedisManager::SyncUserRaidScore;

    RedisRun(RedisThreadCnt_);

    channelServersManager = new ChannelServersManager;
    channelServersManager->init();
}

void RedisManager::SetManager(ConnUsersManager* connUsersManager_, InGameUserManager* inGameUserManager_) {
    connUsersManager = connUsersManager_;
    inGameUserManager = inGameUserManager_;
}

void RedisManager::InitItemData() {
    std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash> itemData;

    if (!mySQLManager->GetEquipmentItemData(itemData)) return;
    if (!mySQLManager->GetConsumableItemData(itemData)) return;
    if (!mySQLManager->GetMaterialItemData(itemData)) return;

    ItemDataManager::GetInstance().LoadFromMySQL(itemData);
}

void RedisManager::InitShopData() {
    std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash> shopItemData;

    if (!mySQLManager->GetShopItemData(shopItemData)) return;

    ShopDataManager::GetInstance().LoadFromMySQL(shopItemData);
}

void RedisManager::InitPassData() {

    // 현재 날짜 기준으로 진행 중인 패스 ID 및 최대 레벨 조회
    std::vector<std::pair<std::string, PassInfo>> passIdVector; // { 패스 ID, 패스 최대 레벨 }
    if (!mySQLManager->GetPassInfo(passIdVector)) return;

    // 각 패스 ID에 해당하는 보상 아이템 데이터 로드
    std::unordered_map<std::string, std::unordered_map<PassDataKey, PassDataForSend, PassDataKeyHash>> passDataMap;
    if (!mySQLManager->GetPassItemData(passIdVector, passDataMap)) return;

    // 패스 레벨별 누적 경험치 정보 로드 (추후 패스별 경험치 테이블이 필요하면 passIdVector를 넘겨 패스별 경험치를 로드하도록 변경)
    std::vector<uint16_t> passExpLimit_;
    if (!mySQLManager->GetPassExpData(passExpLimit_)) return;

    PassRewardManager::GetInstance().LoadFromMySQL(passIdVector, passDataMap, passExpLimit_);
}


// ===================== PACKET MANAGEMENT =====================

void RedisManager::PushRedisPacket(const uint16_t connObjNum_, const uint32_t size_, char* recvData_) {
    ConnUser* TempConnUser = connUsersManager->FindUser(connObjNum_);
    TempConnUser->WriteRecvData(recvData_, size_); // Push Data in Circualr Buffer
    DataPacket tempD(size_, connObjNum_);
    procSktQueue.push(tempD);
}


// ==================== CONNECTION INTERFACE ===================

void RedisManager::Disconnect(uint16_t connObjNum_) {
    if (connUsersManager->FindUser(connObjNum_)->GetPk() == 0) return; // Check the server closed
    UserDisConnect(connObjNum_);
}


// ====================== REDIS MANAGEMENT =====================

void RedisManager::RedisRun(const uint16_t RedisThreadCnt_) { // Connect Redis Server
    try {
        connection_options.host = "127.0.0.1";  // Redis Cluster IP
        connection_options.port = 7001;  // Redis Cluster Master Node Port
        connection_options.socket_timeout = std::chrono::seconds(10);
        connection_options.keep_alive = true;

        redis = std::make_unique<sw::redis::RedisCluster>(connection_options);
        std::cout << "Redis Cluster Connected" << std::endl;

        mySQLManager = new MySQLManager;
        mySQLManager->init();

        InitItemData();
        InitShopData();
        InitPassData();

        CreateRedisThread(RedisThreadCnt_);
    }
    catch (const  sw::redis::Error& err) {
        std::cout << "Redis Connect Error : " << err.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}

bool RedisManager::CreateRedisThread(const uint16_t RedisThreadCnt_) {
    redisRun = true;
    
    try {
        for (int i = 0; i < RedisThreadCnt_; i++) {
            redisThreads.emplace_back(std::thread([this]() { RedisThread(); }));
        }
    }
    catch (const std::system_error& e) {
        std::cerr << "Create Redis Thread Failed : " << e.what() << std::endl;
        return false;
    }

    return true;
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


// ================== SYNCRONIZATION ==================

USERINFO RedisManager::GetUpdatedUserInfo(uint16_t userPk_) {
    std::string userInfokey = "userinfo:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> userData;
    
    USERINFO tempUser;

    try {
        redis->hgetall(userInfokey, std::inserter(userData, userData.begin()));

        tempUser.userId = userData["userId"];
        tempUser.raidScore = std::stoul(userData["raidScore"]);
        tempUser.exp = std::stoul(userData["exp"]);
        tempUser.level = static_cast<uint16_t>(std::stoi(userData["level"]));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get UserInfo for UserPk: " << userPk_ << std::endl;
        return tempUser;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempUser;
}

std::vector<EQUIPMENT> RedisManager::GetUpdatedEquipment(uint16_t userPk_) {
    std::string eqSlot = "equipment:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> equipments;

    std::vector<EQUIPMENT> tempEqv;

    try {
        redis->hgetall(eqSlot, std::inserter(equipments, equipments.begin()));

        for (auto& [key, value] : equipments) {
            size_t div = value.find(':');

            if (div == std::string::npos) {
                std::cout << "Invalid data format in Redis for position: " << key << std::endl;
                continue;
            }

            EQUIPMENT eq;
            eq.position = static_cast<uint16_t>(std::stoi(key));
            eq.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            eq.enhance = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempEqv.emplace_back(eq);
        }
    }
    catch(const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Equipment for UserPk: " << userPk_ << std::endl;
        return tempEqv;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempEqv;
}

std::vector<CONSUMABLES> RedisManager::GetUpdatedConsumables(uint16_t userPk_) {
    std::string csSlot = "consumables:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> consumables;

    std::vector<CONSUMABLES> tempCsv;

    try {
        redis->hgetall(csSlot, std::inserter(consumables, consumables.begin()));

        for (auto& [key, value] : consumables) {
            size_t div = value.find(':');
            if (div == std::string::npos) {
                std::cerr << "Invalid data format in Redis for position: " << key << std::endl;
                continue;
            }

            CONSUMABLES cs;
            cs.position = static_cast<uint16_t>(std::stoi(key));
            cs.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            cs.count = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempCsv.emplace_back(cs);
        }
    }
    catch(const sw::redis::Error& e) {
        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << "Failed to Get Consumables for UserPk: " << userPk_ << std::endl;
        return tempCsv;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }

    return tempCsv;
}

std::vector<MATERIALS> RedisManager::GetUpdatedMaterials(uint16_t userPk_) {
    std::string mtSlot = "materials:{" + std::to_string(userPk_) + "}";
    std::unordered_map<std::string, std::string> materials;

    redis->hgetall(mtSlot, std::inserter(materials, materials.begin()));

    std::vector<MATERIALS> tempMtv;

    for (auto& [key, value] : materials) {
        MATERIALS cs;

        if (value == "") { // Skip if the inventory slot is empty
            tempMtv.emplace_back(cs);
            continue;
        }

        size_t div = value.find(':');
        if (div == std::string::npos) {
            std::cerr << "Invalid data format in Redis for position : " << key << std::endl;
            continue;
        }

        try {
            cs.position = static_cast<uint16_t>(std::stoi(key));
            cs.itemCode = static_cast<uint16_t>(std::stoi(value.substr(0, div)));
            cs.count = static_cast<uint16_t>(std::stoi(value.substr(div + 1)));

            tempMtv.emplace_back(cs);
        }
        catch(const sw::redis::Error& e) {
            std::cerr << "Redis error: " << e.what() << std::endl;
            std::cout << "Failed to Get Materials for UserPk: " << userPk_ << std::endl;
            continue;
        }
        catch (const std::exception& e) {
            std::cerr << "Exception error : " << e.what() << std::endl;
        }
    }

    return tempMtv;
}


// ======================================================= CENTER SERVER =======================================================

void RedisManager::UserConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto userConn = reinterpret_cast<USER_CONNECT_REQUEST_PACKET*>(pPacket_);
    std::string key = "jwtcheck:{" + (std::string)userConn->userId + "}";

    USER_CONNECT_RESPONSE_PACKET ucReq;
    ucReq.PacketId = (uint16_t)PACKET_ID::USER_CONNECT_RESPONSE;
    ucReq.PacketLength = sizeof(USER_CONNECT_RESPONSE_PACKET);

    try {
        auto pk = static_cast<uint32_t>(std::stoul(*redis->hget(key, (std::string)userConn->userToken)));

        if (pk) {
            std::string key = "userinfo:{" + std::to_string(pk) + "}";
            std::unordered_map<std::string, std::string> userData;
            redis->hgetall(key, std::inserter(userData, userData.begin()));

            auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

            connUsersManager->FindUser(connObjNum_)->SetPk(pk);
            tempUser->Set(pk, std::stoul(userData["exp"]),
            static_cast<uint16_t>(std::stoul(userData["level"])), std::stoul(userData["raidScore"]),(std::string)userConn->userId);

            redis->hset(key, "userstate", "online"); // Set user status to "online" in Redis Cluster
            tempUser->SetUserState(UserState::online); // Set user status to "online" in InGameUser object

            ucReq.isSuccess = true;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);

            std::cout << (std::string)userConn->userId << " Authentication Successful" << std::endl;
        }
        else {
            ucReq.isSuccess = false;
            connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);

            std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;
        }
    }
    catch (const sw::redis::Error& e) {
        ucReq.isSuccess = false;
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(USER_CONNECT_RESPONSE_PACKET), (char*)&ucReq);

        std::cerr << "Redis error: " << e.what() << std::endl;
        std::cout << (std::string)userConn->userId << " Authentication Failed" << std::endl;

        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}

void RedisManager::UserDisConnect(uint16_t connObjNum_) {
    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);
    auto tempPk = tempUser->GetPk();

    try {
        redis->hset("userinfo:{" + std::to_string(tempPk) + "}", "userstate", "offline"); // Set user status to "offline" in Redis Cluster
        mySQLManager->LogoutSync(tempPk, GetUpdatedUserInfo(tempPk), 
            GetUpdatedEquipment(tempPk), GetUpdatedConsumables(tempPk), GetUpdatedMaterials(tempPk));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}

void RedisManager::SendServerUserCounts(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
	SERVER_USER_COUNTS_RESPONSE serverUserCountsResPacket;
    serverUserCountsResPacket.PacketId = (uint16_t)PACKET_ID::SERVER_USER_COUNTS_RESPONSE;

    const auto& tempV = channelServersManager->GetServerCounts();
    uint16_t count = static_cast<uint16_t>(tempV.size());

    uint16_t validServerCount = 0;
    for (int i = 1; i < count && validServerCount < MAX_SERVER_USERS; ++i) {
        serverUserCountsResPacket.serverUserCnt[validServerCount++] = tempV[i];
    }

    serverUserCountsResPacket.serverCount = validServerCount;
    serverUserCountsResPacket.PacketLength = sizeof(uint16_t) * 3 + sizeof(uint16_t) * validServerCount;

    try {
        redis->hset("userinfo:{" + std::to_string(inGameUserManager->GetInGameUserByObjNum(connObjNum_)->GetPk()) + "}", "userstate", "serverSwitching"); // Set user status to "serverSwitching" in Redis Cluster
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(serverUserCountsResPacket.PacketLength, reinterpret_cast<char*>(&serverUserCountsResPacket));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}

void RedisManager::SendShopDataToClient(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    const auto& shopVector = ShopDataManager::GetInstance().GetShopData();
    uint16_t shopVectorSize = static_cast<uint16_t>(shopVector.size());
    
    // 전송할 전체 버퍼 크기
    size_t packetSize = sizeof(SHOP_DATA_RESPONSE) + sizeof(ShopItemForSend) * shopVectorSize;
    char* packetBuffer = new char[packetSize];

    auto* shopDataResPacket = reinterpret_cast<SHOP_DATA_RESPONSE*>(packetBuffer);
    shopDataResPacket->PacketId = static_cast<uint16_t>(PACKET_ID::SHOP_DATA_RESPONSE);
    shopDataResPacket->PacketLength = static_cast<uint16_t>(packetSize);
    shopDataResPacket->shopItemSize = shopVectorSize;

    ShopItemForSend* itemVector = reinterpret_cast<ShopItemForSend*>(packetBuffer + sizeof(SHOP_DATA_RESPONSE));

    for (int i = 0; i < shopVectorSize; ++i) {
        itemVector[i] = shopVector[i];
    }

    try {
        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(static_cast<uint16_t>(packetSize), packetBuffer);
    }
    catch (const std::exception& e) {
        std::cerr << "[SendShopDataToClient] Exception: " << e.what() << '\n';
    }

    delete[] packetBuffer;
}

void RedisManager::ChannelDisConnect(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<USER_DISCONNECT_AT_CHANNEL_REQUEST*>(pPacket_);

    channelServersManager->LeaveChannelServer(MoveCHReqPacket->channelServerNum);
}

void RedisManager::MoveServer(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto MoveCHReqPacket = reinterpret_cast<MOVE_SERVER_REQUEST*>(pPacket_);

    MOVE_SERVER_RESPONSE moveCHResPacket;
    std::string tag;

	auto moveServerNum = MoveCHReqPacket->serverNum + CHANNEL_SERVER_START_NUMBER;

    moveCHResPacket.PacketId = (uint16_t)PACKET_ID::MOVE_SERVER_RESPONSE;
    moveCHResPacket.PacketLength = sizeof(MOVE_SERVER_RESPONSE);
    moveCHResPacket.port = ServerAddressMap[static_cast<ServerType>(moveServerNum)].port;
    strncpy_s(moveCHResPacket.ip, ServerAddressMap[static_cast<ServerType>(moveServerNum)].ip.c_str(), 256);

    if (!channelServersManager->EnterChannelServer(moveServerNum)) { // Failed to move to channel server 1
        moveCHResPacket.port = 0;
        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        return;
    };

    auto tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    try { // Generate JWT token
        std::string token = jwt::create()
            .set_issuer("Center_Server")
            .set_subject("Move_Server")
            .set_payload_claim("user_id", jwt::claim(tempUser->GetId()))
            .set_expires_at(std::chrono::system_clock::now() +
                std::chrono::seconds{ 300 })
            .sign(jwt::algorithm::hs256{ JWT_SECRET });
        
        tag = "{" + std::to_string(moveServerNum) + "}";
        std::string key = "jwtcheck:" + tag;

        auto pipe = redis->pipeline(tag);
        pipe.hset(key, token, std::to_string(tempUser->GetPk()))
            .expire(key, 300);
        pipe.exec();

        strncpy_s(moveCHResPacket.serverToken, token.c_str(), MAX_JWT_TOKEN_LEN + 1);
        connUsersManager->FindUser(connObjNum_)-> // Send Channel Server address and JWT token to the user
            PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
    }
    catch (const sw::redis::Error& e) { 
        std::cerr << "Redis error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(moveCHResPacket.serverToken, 0, sizeof(moveCHResPacket.serverToken));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
        return;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception error : " << e.what() << std::endl;

        // Send 0 when JWT token generation fails
        memset(moveCHResPacket.serverToken, 0, sizeof(moveCHResPacket.serverToken));
        connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MOVE_SERVER_RESPONSE), (char*)&moveCHResPacket);
    }
}

void RedisManager::BuyItemFromShop(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto buyItemReq = reinterpret_cast<SHOP_BUY_ITEM_REQUEST*>(pPacket_);

    SHOP_BUY_ITEM_RESPONSE shopBuyRes;
    shopBuyRes.PacketId = (uint16_t)PACKET_ID::SHOP_BUY_ITEM_RESPONSE;
    shopBuyRes.PacketLength = sizeof(SHOP_BUY_ITEM_RESPONSE);

    ConnUser* user = connUsersManager->FindUser(connObjNum_);
    std::string currencyTypeKey = "userinfo:{" + std::to_string(user->GetPk()) + "}";

    auto itemInfo = ShopDataManager::GetInstance().GetItem(buyItemReq->itemCode, buyItemReq->daysOrCount);
    if (!itemInfo) {
        std::cerr << "[BuyItemFromShop] Unknown Item" << '\n';
        shopBuyRes.isSuccess = false;
        user->PushSendMsg(sizeof(shopBuyRes), (char*)&shopBuyRes);
    }

    auto tempItemType = itemInfo->itemType;
    shopBuyRes.passCurrencyType = itemInfo->itemType;

    auto BuyItemFail = [&]() {
        shopBuyRes.isSuccess = false;
        user->PushSendMsg(sizeof(shopBuyRes), (char*)&shopBuyRes);
    };

    if (currencyTypeMap.find(tempItemType) == currencyTypeMap.end()) {
        std::cerr << "[BuyItemFromShop] Unknown currency type" << '\n';
        BuyItemFail();
        return;
    }

    uint32_t userMoney = 0;
    auto moneyType = currencyTypeMap.at(tempItemType);

    try {
        auto val = redis->hget(currencyTypeKey, moneyType);
        if (!val) {
            std::cerr << "[BuyItemFromShop] Redis key not found : " << currencyTypeKey << '\n';
            BuyItemFail();
            return;
        }
        userMoney = std::stoul(*val);
    }
    catch (const std::exception& e) {
        std::cerr << "[BuyItemFromShop] Redis get failed : " << e.what() << '\n';
        BuyItemFail();
        return;
    }

    if (userMoney < itemInfo->itemPrice) {
        BuyItemFail();
        return;
    }

    std::string tag = "{" + std::to_string(user->GetPk()) + "}";
    std::string invenKey;

    if (buyItemReq->itemType == 0) { // 장비
        invenKey = "equipment:" + tag;
    }
    else if (buyItemReq->itemType == 1) { // 소비
        invenKey = "consumables:" + tag;
    }
    else if (buyItemReq->itemType == 2) { // 재료
        invenKey = "materials:" + tag;
    }

    try {
        // 유저 금액 차감
        redis->hincrby(currencyTypeKey, moneyType, -static_cast<int64_t>(itemInfo->itemPrice));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "[BuyItemFromShop] Redis failed : " << e.what() << '\n';
        BuyItemFail();
        return;
    }

    try {
        // 인벤토리에 아이템 추가
        redis->hset(invenKey, std::to_string(buyItemReq->itemType), std::to_string(itemInfo->itemCode) + ":" + std::to_string(itemInfo->daysOrCount));
    }
    catch (const sw::redis::Error& e) {
        // 인벤토리 삽입 실패 시, 차감한 금액 복구
        redis->hincrby(currencyTypeKey, moneyType, itemInfo->itemPrice);

        std::cerr << "[BuyItemFromShop] Redis failed : " << e.what() << '\n';
        BuyItemFail();
        return;
    }

    // MySQL 트랜잭션 실행 (금액 차감 + 아이템 삽입)
    bool dbSuccess = mySQLManager->BuyItem(itemInfo->itemCode, itemInfo->daysOrCount, buyItemReq->itemType,
        itemInfo->currencyType, user->GetPk(), itemInfo->itemPrice);

    if (!dbSuccess) { 
        // MySQL 트랜잭션 실패 시 Redis 상태 복원 (금액 복구 및 아이템 제거)
        redis->hincrby(currencyTypeKey, moneyType, itemInfo->itemPrice);
        redis->hdel(invenKey, std::to_string(buyItemReq->itemType));

        BuyItemFail();
        return;
    }

    // 아이템 구매 성공
    shopBuyRes.isSuccess = true;
    shopBuyRes.remainMoney = (userMoney - itemInfo->itemPrice);
    user->PushSendMsg(sizeof(shopBuyRes), (char*)&shopBuyRes);
}

void RedisManager::SendPassDataToClient(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    const auto& passVector = PassRewardManager::GetInstance().GetPassData();
    uint16_t passVectorSize = static_cast<uint16_t>(passVector.size());

    size_t packetSize = sizeof(PASS_DATA_RESPONSE) + sizeof(PassDataForSend) * passVectorSize;
    char* packetBuffer = new char[packetSize];

    auto* passDataResPacket = reinterpret_cast<PASS_DATA_RESPONSE*>(packetBuffer);
    passDataResPacket->PacketId = static_cast<uint16_t>(PACKET_ID::PASS_DATA_RESPONSE);
    passDataResPacket->PacketLength = static_cast<uint16_t>(packetSize);
    passDataResPacket->passDataSize = passVectorSize;

    PassDataForSend* passVectorForSend = reinterpret_cast<PassDataForSend*>(packetBuffer + sizeof(PASS_DATA_RESPONSE));

    for (int i = 0; i < passVectorSize; i++) {
        passVectorForSend[i] = passVector[i];
    }

    try {
        connUsersManager->FindUser(connObjNum_)->
            PushSendMsg(static_cast<uint16_t>(packetSize), packetBuffer);
    }
    catch (const std::exception& e) {
        std::cerr << "[SendPassDataToClient] Exception: " << e.what() << '\n';
    }

    delete[] packetBuffer;
}

void RedisManager::PassExpUp(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto expUpReqPacket = reinterpret_cast<PASS_EXP_UP_REQUEST*>(pPacket_);

    auto tempPassId = (std::string)expUpReqPacket->passId;

    ConnUser* user = connUsersManager->FindUser(connObjNum_);
    auto tempUserPk = user->GetPk();
    auto* tempPassData = PassRewardManager::GetInstance().GetPassItemDataByPassId(tempPassId, expUpReqPacket->passLevel, expUpReqPacket->passCurrencyType);

    PASS_EXP_UP_RESPONSE expUpResPacket;
    expUpResPacket.PacketId = (uint16_t)PACKET_ID::PASS_EXP_UP_RESPONSE;
    expUpResPacket.PacketLength = sizeof(PASS_EXP_UP_RESPONSE);

    auto tempPassCurrencyType = static_cast<PassCurrencyType>(tempPassData->passCurrencyType);

    auto PassExpUpFail = [&]() {
        expUpResPacket.isSuccess = false;
        strcpy_s(expUpResPacket.passId, expUpReqPacket->passId);
        user->PushSendMsg(sizeof(expUpResPacket), (char*)&expUpResPacket);
    };

    if (PassCurrencyTypeMap.find(tempPassCurrencyType) == PassCurrencyTypeMap.end()) {
        std::cerr << "[PassExpUp] Unknown passCurrency type" << '\n';
        PassExpUpFail();
        return;
    }

    std::string passCurrencyType = PassCurrencyTypeMap.at(tempPassCurrencyType);
    std::string passKey = "pass:{" + std::to_string(tempUserPk) + "}:" + tempPassId + ":" + passCurrencyType;

    std::pair<uint16_t, uint16_t> tempPassLevelandExp; // {레벨 증가량, 현재 경험치}

    // 유저 패스 경험치 증가 요청
    try {
        auto passLevelVal = redis->hget(passKey, "userPassLevel");
        if (!passLevelVal) {
            std::cerr << "[PassExpUp] Redis key not found(userPassLevel) : " << passKey << '\n';
            PassExpUpFail();
            return;
        }

        auto passExpVal = redis->hget(passKey, "userPassExp");
        if (!passExpVal) {
            std::cerr << "[PassExpUp] Redis key not found(userPassExp) : " << passKey << '\n';
            PassExpUpFail();
            return;
        }

        tempPassLevelandExp = PassRewardManager::GetInstance().PassExpUp(expUpReqPacket->acqPassExp, std::stoi(*passLevelVal), std::stoi(*passExpVal));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "[PassExpUp] Redis key not found(userPassExp) : " << passKey << '\n';
        PassExpUpFail();
        return;
    }

    auto currentUserLevel = expUpReqPacket->passLevel + tempPassLevelandExp.first;
    auto currentUserExp = tempPassLevelandExp.second;

    if (tempPassLevelandExp.first != 0) { // 레벨업
        try {
            auto pipe = redis->pipeline(std::to_string(tempUserPk));

            pipe.hset(passKey, "userPassExp", std::to_string(currentUserExp))
                .hincrby(passKey, "userPassLevel", tempPassLevelandExp.first);

            pipe.exec();

            expUpResPacket.isSuccess = true;
            strcpy_s(expUpResPacket.passId, expUpReqPacket->passId);
            expUpResPacket.passLevel = currentUserLevel;
            expUpResPacket.passExp = currentUserExp;

            user->PushSendMsg(sizeof(PASS_EXP_UP_RESPONSE), (char*)&expUpResPacket);
        }
        catch (const sw::redis::Error& e) {
            std::cerr << "[PassExpUp] Redis error: " << e.what() << std::endl;
            PassExpUpFail();
            return;
        }
    }
    else { // 경험치만 증가
        try {
            if (redis->hincrby(passKey, "userPassExp", expUpReqPacket->acqPassExp)) {

                expUpResPacket.isSuccess = true;
                strcpy_s(expUpResPacket.passId, expUpReqPacket->passId);
                expUpResPacket.passLevel = currentUserLevel;
                expUpResPacket.passExp = currentUserExp;

                user->PushSendMsg(sizeof(PASS_EXP_UP_RESPONSE), (char*)&expUpResPacket);
            }
            else { 
                PassExpUpFail();
                return;
            }
        }
        catch (const sw::redis::Error& e) {
            std::cerr << "[PassExpUp] Redis error: " << e.what() << std::endl;
            PassExpUpFail();
            return;
        }
    }
}

void RedisManager::GetPassItem(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto passReqPacket = reinterpret_cast<GET_PASS_ITEM_REQUEST*>(pPacket_);
    auto tempPassId = (std::string)passReqPacket->passId;

    ConnUser* user = connUsersManager->FindUser(connObjNum_);
    auto* tempPassData = PassRewardManager::GetInstance().GetPassItemDataByPassId(tempPassId, passReqPacket->passLevel, passReqPacket->passCurrencyType);

    GET_PASS_ITEM_RESPONSE getPassRes;
    getPassRes.PacketId = (uint16_t)PACKET_ID::GET_PASS_ITEM_RESPONSE;
    getPassRes.PacketLength = sizeof(GET_PASS_ITEM_RESPONSE);

    auto tempPassCurrencyType = static_cast<PassCurrencyType>(tempPassData->passCurrencyType);

    auto GetPassFail = [&]() {
        getPassRes.isSuccess = false;
        user->PushSendMsg(sizeof(getPassRes), (char*)&getPassRes);
    };

    if (PassCurrencyTypeMap.find(tempPassCurrencyType) == PassCurrencyTypeMap.end()) {
        std::cerr << "[GetPassItem] Unknown passCurrency type" << '\n';
        GetPassFail();
        return;
    }

    std::string passCurrencyType = PassCurrencyTypeMap.at(tempPassCurrencyType);
    std::string passKey = "pass:{" + std::to_string(user->GetPk()) + "}:" + tempPassId + ":" + passCurrencyType;

    // 유저 현재 패스 레벨과 요청한 패스 레벨 체크
    try {
        auto val = redis->hget(passKey, "userPassLevel");
        if (!val) {
            std::cerr << "[GetPassItem] Redis key not found : " << passKey << '\n';
            GetPassFail();
            return;
        }

        if (std::stoi(*val) < passReqPacket->passLevel) { // 유저가 요청한 패스 레벨이 현재 레벨 보다 높다면 false 전송
            GetPassFail();
            return;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GetPassItem] Redis get failed : " << e.what() << '\n';
        GetPassFail();
        return;
    }
    
    std::string tag = "{" + std::to_string(user->GetPk()) + "}";
    std::string invenKey;

    if (tempPassData->itemType == 0) { // 장비
        invenKey = "equipment:" + tag;
    }
    else if (tempPassData->itemType == 1) { // 소비
        invenKey = "consumables:" + tag;
    }
    else if (tempPassData->itemType == 2) { // 재료
        invenKey = "materials:" + tag;
    }

    try {
        // 인벤토리에 아이템 추가
        redis->hset(invenKey, std::to_string(tempPassData->itemType), std::to_string(tempPassData->itemCode) + ":" + std::to_string(tempPassData->daysOrCount));
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "[GetPassItem] Redis failed : " << e.what() << '\n';
        GetPassFail();
        return;
    }

    // MySQL 트랜잭션 실행 (패스 아이템 획득 체크 + 패스 아이템 인벤토리 추가)
    bool dbSuccess = mySQLManager->UpdatePassItem(passReqPacket->passId, user->GetPk(), passReqPacket->passLevel, passReqPacket->passCurrencyType,
        tempPassData->itemCode, tempPassData->daysOrCount, tempPassData->itemType);

    if (!dbSuccess) { // 해당 패스 아이템 이미 받았거나 업데이트 실패 (레디스에 추가한 아이템 제거)
        redis->hdel(invenKey, std::to_string(tempPassData->itemType));

        GetPassFail();
        return;
    }

    getPassRes.isSuccess = true;
    user->PushSendMsg(sizeof(getPassRes), (char*)&getPassRes);
}


// ======================================================= CASH SERVER =======================================================

void RedisManager::CashServerConnectResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto cashReqPacket = reinterpret_cast<CASH_SERVER_CONNECT_REQUEST*>(pPacket_);

    CASH_SERVER_CONNECT_RESPONSE cashRes;
    cashRes.PacketId = (uint16_t)PACKET_ID::CASH_SERVER_CONNECT_RESPONSE;
    cashRes.PacketLength = sizeof(CASH_SERVER_CONNECT_RESPONSE);
    cashRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(CASH_SERVER_CONNECT_RESPONSE), (char*)&cashRes);

    std::cout << "Cash Server Authentication Successful" << std::endl;
}

void RedisManager::CashChargeResultResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    ConnUser* tempConnUser = connUsersManager->FindUser(connObjNum_);

    if (connObjNum_ != 11) { // Cash Server가 아닌 해커의 부정 요청
        auto tempSocket = tempConnUser->GetSocket();

        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        char ipStr[INET_ADDRSTRLEN] = {};

        if (getpeername(tempSocket, (sockaddr*)&clientAddr, &addrLen) == 0) {
            inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        }
        else {
            std::cerr << "getpeername() failed : " << WSAGetLastError() << std::endl;
        }

        std::cout << "[AbnormalCashRequest] User Pk : " << tempConnUser->GetPk() << " || IP : " << ipStr << std::endl;
        return;
    }

    auto cashChargeResPacket = reinterpret_cast<CASH_CHARGE_RESULT_RESPONSE*>(pPacket_);

    try { // 충전된 금액 Redis와 Mysql에 업데이트
        redis->hincrby("userinfo:{" + std::to_string(tempConnUser->GetPk()) + "}", "usercash", cashChargeResPacket->chargedAmount);
        if (!mySQLManager->CashCharge(tempConnUser->GetPk(), cashChargeResPacket->chargedAmount)) {
            // MYSQL 업데이트 실패시 처리 서버로 전달해서 우선순위로 처리해주기

        }
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
    }
}


// ======================================================= LOGIN SERVER =======================================================

void RedisManager::LoginServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<LOGIN_SERVER_CONNECT_REQUEST*>(pPacket_);

    LOGIN_SERVER_CONNECT_RESPONSE imLRes;
    imLRes.PacketId = (uint16_t)PACKET_ID::LOGIN_SERVER_CONNECT_RESPONSE;
    imLRes.PacketLength = sizeof(LOGIN_SERVER_CONNECT_RESPONSE);
    imLRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(LOGIN_SERVER_CONNECT_RESPONSE), (char*)&imLRes);

    std::cout << "Login Server Authentication Successful" << std::endl;
}


// ======================================================= CHANNEL SERVER =======================================================

void RedisManager::ChannelServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imChReqPacket = reinterpret_cast<CHANNEL_SERVER_CONNECT_REQUEST*>(pPacket_);
	ServerAddressMap[static_cast<ServerType>(imChReqPacket->channelServerNum)].serverObjNum = connObjNum_;

    CHANNEL_SERVER_CONNECT_RESPONSE imChRes;
    imChRes.PacketId = (uint16_t)PACKET_ID::CHANNEL_SERVER_CONNECT_RESPONSE;
    imChRes.PacketLength = sizeof(CHANNEL_SERVER_CONNECT_RESPONSE);
    imChRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0);// Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_RANKING_RESPONSE), (char*)&imChRes);

    std::cout << "Channel Server" << imChReqPacket->channelServerNum << " Authentication Successful" << std::endl;
}

void RedisManager::SyncEqipmentEnhace(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto syncEquipReqPacket = reinterpret_cast<SYNC_EQUIPMENT_ENHANCE_REQUEST*>(pPacket_);

	mySQLManager->MySQLSyncEqipmentEnhace(syncEquipReqPacket->userPk, syncEquipReqPacket->itemPosition , syncEquipReqPacket->enhancement);
}


// ======================================================= MATCHING SERVER =======================================================

void RedisManager::MatchingServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imMatchingReqPacket = reinterpret_cast<MATCHING_SERVER_CONNECT_REQUEST*>(pPacket_);

    ServerAddressMap[ServerType::MatchingServer].serverObjNum = connObjNum_;

    MATCHING_SERVER_CONNECT_RESPONSE imMRes;
    imMRes.PacketId = (uint16_t)PACKET_ID::MATCHING_SERVER_CONNECT_RESPONSE;
    imMRes.PacketLength = sizeof(MATCHING_SERVER_CONNECT_RESPONSE);
    imMRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(MATCHING_SERVER_CONNECT_RESPONSE), (char*)&imMRes);

    std::cout << "Matching Server Authentication Successful" << std::endl;
}

void RedisManager::MatchStart(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) { 
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_REQUEST_TO_MATCHING_SERVER matchReqPacket;
    matchReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_REQUEST_TO_MATCHING_SERVER;
    matchReqPacket.PacketLength = sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER);
    matchReqPacket.userPk = tempUser->GetPk();
	matchReqPacket.userCenterObjNum = connObjNum_;
    matchReqPacket.userGroupNum = tempUser->GetUserGroupNum();

    connUsersManager->FindUser(ServerAddressMap[ServerType::MatchingServer].serverObjNum)->PushSendMsg(sizeof(MATCHING_REQUEST_TO_MATCHING_SERVER), (char*)&matchReqPacket);
}

void RedisManager::MatchStartResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    RAID_MATCHING_RESPONSE matchResPacket;
    matchResPacket.PacketId = (uint16_t)PACKET_ID::RAID_MATCHING_RESPONSE;
    matchResPacket.PacketLength = sizeof(RAID_MATCHING_RESPONSE);

    if (matchSuccessReqPacket->isSuccess) {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;

        try {
            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "raidMatching"); // Set user status to "raidMatching" in Redis Cluster

            tempUser->SetUserState(UserState::raidMatching);
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);
            return;
        }
        catch (const sw::redis::Error& e) {
            matchResPacket.insertSuccess = false;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);

            std::cerr << "Redis error : " << e.what() << std::endl;
            return;
        }
    }
    else {
        matchResPacket.insertSuccess = matchSuccessReqPacket->isSuccess;
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchResPacket);
        std::cout << tempUser->GetId() << " " << tempUser->GetUserGroupNum() << "group matching failed" << std::endl;
    }
}

void RedisManager::MatchingCancel(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(connObjNum_);

    MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER matchCancelReqPacket;
    matchCancelReqPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER;
    matchCancelReqPacket.PacketLength = sizeof(MATCHING_CANCEL_REQUEST_TO_MATCHING_SERVER);
    matchCancelReqPacket.userCenterObjNum = connObjNum_;
    matchCancelReqPacket.userGroupNum = tempUser->GetLevel() / 3 + 1;

    connUsersManager->FindUser(ServerAddressMap[ServerType::MatchingServer].serverObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCancelReqPacket);
}

void RedisManager::MatchingCancelResponse(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchCancelResPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_MATCHING_SERVER*>(pPacket_);
    InGameUser* tempUser = inGameUserManager->GetInGameUserByObjNum(matchCancelResPacket->userCenterObjNum);

    MATCHING_CANCEL_RESPONSE matchCanResPacket;
    matchCanResPacket.PacketId = (uint16_t)PACKET_ID::MATCHING_CANCEL_RESPONSE;
    matchCanResPacket.PacketLength = sizeof(MATCHING_CANCEL_RESPONSE);
    matchCanResPacket.isSuccess = matchCancelResPacket->isSuccess;

    try {
        redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster
    }
    catch (const sw::redis::Error& e) {
        std::cerr << "Redis error : " << e.what() << std::endl;
        return;
    }

    tempUser->SetUserState(UserState::online);

    connUsersManager->FindUser(matchCancelResPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_MATCHING_RESPONSE), (char*)&matchCanResPacket);

    std::cout << tempUser->GetId() << " Matching Cancel Success" << std::endl;
}

void RedisManager::CheckMatchSuccess(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto matchSuccessReqPacket = reinterpret_cast<MATCHING_RESPONSE_FROM_GAME_SERVER*>(pPacket_);

    auto raidServerNum = matchSuccessReqPacket->serverNum + GAME_SERVER_START_NUMBER;
    uint16_t tempRoomNum = matchSuccessReqPacket->roomNum;

    RAID_READY_REQUEST raidReadyReqPacket;
    raidReadyReqPacket.PacketId = (uint16_t)PACKET_ID::RAID_READY_REQUEST;
    raidReadyReqPacket.PacketLength = sizeof(RAID_READY_REQUEST);
    raidReadyReqPacket.roomNum = tempRoomNum;
    raidReadyReqPacket.port = ServerAddressMap[static_cast<ServerType>(raidServerNum)].port;
    strncpy_s(raidReadyReqPacket.ip, ServerAddressMap[static_cast<ServerType>(raidServerNum)].ip.c_str(), 256);

    if (matchSuccessReqPacket->roomNum == 0) { // Send game creation failure message to matched users
        raidReadyReqPacket.roomNum = 0;
        connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
        return;
    }

    auto tempUser = inGameUserManager->GetInGameUserByObjNum(matchSuccessReqPacket->userCenterObjNum);

    { // Generate JWT token
        try {
            std::string tag = "{" + std::to_string(static_cast<uint16_t>(ServerType::RaidGameServer01)) + "}";
            std::string key = "jwtcheck:" + tag;

            std::string token = jwt::create()
                .set_issuer("Center_Server")
                .set_subject("Connect_GameServer")
                .set_payload_claim("user_center_id", jwt::claim(std::to_string(matchSuccessReqPacket->userCenterObjNum))) // User unique ID used in the Center Server
                .set_payload_claim("room_id", jwt::claim(std::to_string(tempRoomNum))) // Matched room number
                .set_payload_claim("raid_id", jwt::claim(std::to_string(matchSuccessReqPacket->userRaidServerObjNum))) // User unique ID used in the Raid Server
                .set_expires_at(std::chrono::system_clock::now() +
                    std::chrono::seconds{ 300 })
                .sign(jwt::algorithm::hs256{ JWT_SECRET });

            auto pipe = redis->pipeline(tag);

            pipe.hset(key, token, std::to_string(tempUser->GetPk())) // Save the assigned Game Server unique ID to Redis
                .expire(key, 60);

            pipe.hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "inRaid"); // Set user status to "inRaid" in Redis Cluster

            pipe.exec();

            strncpy_s(raidReadyReqPacket.serverToken, token.c_str(), MAX_JWT_TOKEN_LEN + 1);

            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            std::cout << "매칭 성공 pk : " << connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->GetPk() << std::endl;
        }
        catch (const sw::redis::Error& e) {
            raidReadyReqPacket.roomNum = 0;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            
            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster

            { // Send failed raid ready userObjNum to the game server
                RAID_READY_FAIL rrF;
                rrF.PacketId = (uint16_t)PACKET_ID::RAID_READY_FAIL;
                rrF.PacketLength = sizeof(RAID_READY_FAIL);
                rrF.userCenterObjNum = matchSuccessReqPacket->userCenterObjNum;
                rrF.roomNum = matchSuccessReqPacket->roomNum;

                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            }

            std::cerr << "Redis error : " << e.what() << std::endl;
            return;
        }
        catch (const std::exception& e) {
            raidReadyReqPacket.roomNum = 0;
            connUsersManager->FindUser(matchSuccessReqPacket->userCenterObjNum)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);

            redis->hset("userinfo:{" + std::to_string(tempUser->GetPk()) + "}", "userstate", "online"); // Set user status to "online" in Redis Cluster

            { // Send failed raid ready userObjNum to the game server
                RAID_READY_FAIL rrF;
                rrF.PacketId = (uint16_t)PACKET_ID::RAID_READY_FAIL;
                rrF.PacketLength = sizeof(RAID_READY_FAIL);
                rrF.userCenterObjNum = matchSuccessReqPacket->userCenterObjNum;
                rrF.roomNum = matchSuccessReqPacket->roomNum;

                connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_READY_REQUEST), (char*)&raidReadyReqPacket);
            }

            std::cerr << "Exception error : " << e.what() << std::endl;
            return;
        }
    }
}


// ======================================================= RAID GAME SERVER =======================================================

void RedisManager::GameServerConnectRequest(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto imGameReqPacket = reinterpret_cast<RAID_SERVER_CONNECT_REQUEST*>(pPacket_);
    ServerAddressMap[static_cast<ServerType>(imGameReqPacket->gameServerNum)].serverObjNum = connObjNum_;

    RAID_SERVER_CONNECT_RESPONSE imGameRes;
    imGameRes.PacketId = (uint16_t)PACKET_ID::RAID_SERVER_CONNECT_RESPONSE;
    imGameRes.PacketLength = sizeof(RAID_SERVER_CONNECT_RESPONSE);
    imGameRes.isSuccess = true;

    connUsersManager->FindUser(connObjNum_)->SetPk(0); // Set server PK to 0 for servers connected to the center server
    connUsersManager->FindUser(connObjNum_)->PushSendMsg(sizeof(RAID_SERVER_CONNECT_RESPONSE), (char*)&imGameRes);

    std::cout << "Game Server" << imGameReqPacket->gameServerNum - GAME_SERVER_START_NUMBER << " Authentication Successful" << std::endl;
}

void RedisManager::SyncUserRaidScore(uint16_t connObjNum_, uint16_t packetSize_, char* pPacket_) {
    auto delEquipReqPacket = reinterpret_cast<SYNC_HIGHSCORE_REQUEST*>(pPacket_);

    try {
        mySQLManager->MySQLSyncUserRaidScore(delEquipReqPacket->userPk, delEquipReqPacket->userScore, std::string(delEquipReqPacket->userId));
    }
    catch (const std::exception& e) {
        std::cout << "(MySQL Sync Fail) UserPK : " << delEquipReqPacket->userPk << ", Score: " << delEquipReqPacket->userScore << std::endl;
        std::cerr << "Exception error : " << e.what() << std::endl;
    }
}