#include "MySQLManager.h"

// ====================== INITIALIZATION =======================

bool MySQLManager::init() {
    for (int i = 0; i < dbConnectionCount; i++) {
        MYSQL* Conn = mysql_init(nullptr);
        if (!Conn) {
            std::cerr << "Mysql Init Fail" << std::endl;
            return false;
        }

        MYSQL* ConnPtr = mysql_real_connect(Conn, DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT, (char*)NULL, 0);
        if (!ConnPtr) {
            std::cerr << "Mysql Connection Fail : " << mysql_error(Conn) << '\n';
            return false;
        }

        dbPool.push(ConnPtr);
    }

    std::cout << "Mysql Connection Success" << std::endl;
    return true;
}

bool MySQLManager::GetEquipmentItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetEquipmentItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName, attackPower FROM EquipmentData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetEquipmentItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetEquipmentItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2]) continue;

            auto equipmentData = std::make_unique<EquipmentItemData>();
            equipmentData->itemCode = (uint16_t)std::stoi(Row[0]);
            equipmentData->itemName = Row[1];
            equipmentData->attackPower = (uint16_t)std::stoi(Row[2]);
            equipmentData->itemType = ItemType::EQUIPMENT;

            itemData_[{equipmentData->itemCode, static_cast<uint16_t>(equipmentData->itemType)}] = std::move(equipmentData);
        }

        mysql_free_result(Result);

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetEquipmentItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetConsumableItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetConsumableItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName FROM ConsumableData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ConsumableItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ConsumableItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            auto consumableData = std::make_unique<ConsumableItemData>();
            consumableData->itemCode = (uint16_t)std::stoi(Row[0]);
            consumableData->itemName = Row[1];
            consumableData->itemType = ItemType::CONSUMABLE;

            itemData_[{consumableData->itemCode, static_cast<uint16_t>(consumableData->itemType)}] = std::move(consumableData);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[ConsumableItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetMaterialItemData(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& itemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetMaterialItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemName FROM MaterialData";
    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[MaterialItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[MaterialItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            auto materialData = std::make_unique<MaterialItemData>();
            materialData->itemCode = (uint16_t)std::stoi(Row[0]);
            materialData->itemName = Row[1];
            materialData->itemType = ItemType::MATERIAL;

            itemData_[{materialData->itemCode, static_cast<uint16_t>(materialData->itemType)}]= std::move(materialData);
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MaterialItemData] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

bool MySQLManager::GetShopItemData(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemData_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[GetShopItemData] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    std::string query_s = "SELECT item_code, itemType, itemPrice, itemCount, daysOrCount, currencyType FROM ShopItemData";

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ShopEquipmentItem] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return false;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ShopEquipmentItem] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

            ShopItem shopItemData;
            shopItemData.itemCode = (uint16_t)std::stoi(Row[0]);
            shopItemData.itemType = static_cast<ItemType>(std::stoi(Row[1]));
            shopItemData.itemPrice = static_cast<uint32_t>(std::stoul(Row[2]));
            shopItemData.itemCount = (uint16_t)std::stoi(Row[3]);
            shopItemData.daysOrCount = (uint16_t)std::stoi(Row[4]);
            shopItemData.currencyType = static_cast<CurrencyType>(std::stoi(Row[5]));

            shopItemData_[{shopItemData.itemCode, shopItemData.daysOrCount}] = shopItemData;
        }

        mysql_free_result(Result);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[ShopEquipmentItem] Exception Error : " << e.what() << std::endl;
        return false;
    }
}

MYSQL* MySQLManager::GetConnection() {
    MYSQL* ConnPtr;
    {
        std::lock_guard<std::mutex> lock(dbPoolMutex);
        if (dbPool.empty()) {
            return nullptr;
        }
        ConnPtr = dbPool.front();
        dbPool.pop();
    }
    return ConnPtr;
};


// ======================= SYNCRONIZATION =======================

bool MySQLManager::LogoutSync(uint32_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, 
    std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[LogoutSync] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    mysql_autocommit(ConnPtr, false); // Transaction start

    for (int i = 0; i < 3; i++) { // // Retry up to 3 times on failure
        if (!SyncUserInfo(userPk_, userInfo_)) { std::cout << "SyncUserInfo failed" << '\n'; }
        else if (!SyncEquipment(userPk_, userEquip_)) { std::cout << "SyncEquipment failed" << '\n'; }
        else if (!SyncConsumables(userPk_, userConsum_)) { std::cout << "SyncConsumables failed" << '\n'; }
        else if (!SyncMaterials(userPk_, userMat_)) { std::cout << "SyncMaterials failed" << '\n'; }
        else {
            if (mysql_commit(ConnPtr) == 0) { // If commit is successful, exit
                mysql_autocommit(ConnPtr, true);
                return true;
            }
            else { // If commit fails, rollback
                std::cerr << "mysql_commit failed" << '\n';
                mysql_rollback(ConnPtr);
            }
        }
        mysql_rollback(ConnPtr);
        std::cerr << "userPk : " << userPk_ << " LogoutSync attempt : " << i + 1 << '\n';
    }

    mysql_autocommit(ConnPtr, true);

    // 실패 시 처리 해주는 서버 생성하여 해당 서버로 정보 전달
    //

    std::cerr << "(LogoutSync Failed) userPk : " << userPk_ << '\n';
    return false;
}

bool MySQLManager::SyncUserInfo(uint32_t userPk_, USERINFO userInfo_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncUserInfo] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        std::string query_s = "UPDATE USERS left join Ranking r on USERS.name = r.name SET USERS.name = '" +
            userInfo_.userId + "', USERS.exp = " + std::to_string(userInfo_.exp) +
            ", USERS.level = " + std::to_string(userInfo_.level) + ", USERS.last_login = current_timestamp" +
            ", USERS.server = " + std::to_string(0) + ", USERS.channel = " + std::to_string(0) +
            ",r.score = " + std::to_string(userInfo_.raidScore) +
            " WHERE USERS.id = " + std::to_string(userPk_);

        const char* Query = query_s.c_str();

        MysqlResult = mysql_query(ConnPtr, Query);
        if (MysqlResult != 0) {
            std::cerr << "(SyncUserInfo) MySQL UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncUserInfo) Failed to Sync userPk : " << userPk_ << " UserInfo Data" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized UserInfo with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncEquipment(uint32_t userPk_, std::vector<EQUIPMENT> userEquip_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncEquipment] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        std::ostringstream query_s;
        query_s << "UPDATE Equipment SET ";

        std::ostringstream item_code_case, enhancement_case, where;
        item_code_case << "Item_code = CASE ";
        enhancement_case << "enhance = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;

        for (auto& tempEquip : userEquip_) {

            item_code_case << "WHEN position = " << tempEquip.position << " THEN " << tempEquip.itemCode << " ";
            enhancement_case << "WHEN position = " << tempEquip.position << " THEN " << tempEquip.enhance << " ";

            if (!first) where << ", ";
            where << tempEquip.position;
            first = false;
        }

        item_code_case << "END, ";
        enhancement_case << "END ";
        where << ");";

        query_s << item_code_case.str() << enhancement_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncEquipment) MySQL Batch UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncEquipment) Failed to Sync userPk : " << userPk_ << " Equipments" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Equipment with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncConsumables(uint32_t userPk_, std::vector<CONSUMABLES> userConsum_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncConsumables] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);

    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        std::ostringstream query_s;
        query_s << "UPDATE Consumables SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "daysOrCount = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;
        for (auto& tempConum : userConsum_) { // key = Pos, value = (code, count)

            item_code_case << "WHEN position = " << tempConum.position << " THEN " << tempConum.itemCode << " ";
            count_case << "WHEN position = " << tempConum.position << " THEN " << tempConum.count << " ";

            if (!first) where << ", ";
            where << tempConum.position;
            first = false;
        }

        item_code_case << "END, ";
        count_case << "END ";
        where << ");";

        query_s << item_code_case.str() << count_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncConsumables) CONSUMABLE UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncConsumables) Failed to Sync userPk : " << userPk_ << " Consumables (MySQL or Unknown Error)" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Consumables with MySQL" << std::endl;
    return true;
}

bool MySQLManager::SyncMaterials(uint32_t userPk_, std::vector<MATERIALS> userMat_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[SyncMaterials] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);
    
    MYSQL_RES* Result;
    MYSQL_ROW Row;
    
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Materials SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "daysOrCount = CASE ";

        where << "WHERE user_pk = " << std::to_string(userPk_) << " AND position IN (";

        bool first = true;
        for (auto& tempMat : userMat_) {

            item_code_case << "WHEN position = " << tempMat.position << " THEN " << tempMat.itemCode << " ";
            count_case << "WHEN position = " << tempMat.position << " THEN " << tempMat.count << " ";

            if (!first) where << ", ";
            where << tempMat.position;
            first = false;
        }

        item_code_case << "END, ";
        count_case << "END ";
        where << ");";

        query_s << item_code_case.str() << count_case.str() << where.str();
        if (mysql_query(ConnPtr, query_s.str().c_str()) != 0) {
            std::cerr << "(SyncMaterials) MATERIALS UPDATE Error : " << mysql_error(ConnPtr) << std::endl;
            return false;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "(SyncMaterials) Failed to Sync userPk : " << userPk_ << " Materials (MySQL or Unknown Error)" << std::endl;
        return false;
    }

    std::cout << "Successfully Synchronized Materials with MySQL" << std::endl;
    return true;
}

bool MySQLManager::MySQLSyncEqipmentEnhace(uint32_t userPk_, uint16_t itemPosition_, uint16_t enhancement_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[MySQLSyncEqipmentEnhace] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);
    
    MYSQL_RES* Result;
    MYSQL_ROW Row;

    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE Equipment SET position = ?, enhance = ? WHERE user_pk = ?;";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[3];
        memset(bind, 0, sizeof(bind));

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = &itemPosition_;

        bind[1].buffer_type = MYSQL_TYPE_LONG;
        bind[1].buffer = &enhancement_;

        bind[2].buffer_type = MYSQL_TYPE_LONG;
        bind[2].buffer = &userPk_;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Bind Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "(MySQLSyncEquipmentEnhance) Equipment Enhance Sync Execute Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "(MySQLSyncEquipmentEnhance) Failed to sync equipment enhancement. userPk : " << userPk_ 
            << ", position : " << itemPosition_ << ", enhancement : " << enhancement_ << std::endl;
        return false;
    }
}

bool MySQLManager::MySQLSyncUserRaidScore(uint32_t userPk_, unsigned int userScore_, std::string userId_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[MySQLSyncUserRaidScore] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);
    
    MYSQL_RES* Result;
    MYSQL_ROW Row;
    
    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE Ranking SET score = ? WHERE id = ?;";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Prepare Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[2];
        memset(bind, 0, sizeof(bind));

        unsigned long idLength = userId_.length();

        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = &userScore_;

        bind[1].buffer_type = MYSQL_TYPE_STRING;
        bind[1].buffer = (void*)userId_.c_str();
        bind[1].buffer_length = idLength;
        bind[1].length = &idLength;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Bind Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "(MySQLSyncUserRaidScore) Raid Score Sync Execute Error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "(MySQLSyncUserRaidScore) Failed to sync raid score. userId : " << userId_
            << ", score : " << userScore_ << std::endl;
        return false;
    }
}

bool MySQLManager::CashCharge(uint32_t userPk_, uint32_t chargedAmount) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[CashCharge] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);
    
    MYSQL_RES* Result;
    MYSQL_ROW Row;
    
    try {
        MYSQL_STMT* stmt = mysql_stmt_init(ConnPtr);

        std::string query = "UPDATE USERS SET cash = cash + ? WHERE id = ?";
        if (mysql_stmt_prepare(stmt, query.c_str(), query.length()) != 0) {
            std::cerr << "[CashCharge] Statement prepare error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        MYSQL_BIND bind[2] = {};
        bind[0].buffer_type = MYSQL_TYPE_LONG;
        bind[0].buffer = (char*)&chargedAmount;

        bind[1].buffer_type = MYSQL_TYPE_LONG;
        bind[1].buffer = (char*)&userPk_;

        if (mysql_stmt_bind_param(stmt, bind) != 0) {
            std::cerr << "[CashCharge] Bind error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::cerr << "[CashCharge] Execute error : " << mysql_stmt_error(stmt) << std::endl;
            return false;
        }

        mysql_stmt_close(stmt);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[CashCharge] Exception : " << e.what() << " (UserPk: " << userPk_ << ")" << '\n';
        return false;
    }
}

bool MySQLManager::BuyItem(uint16_t itemCode, uint16_t daysOrCounts_, uint16_t itemType_, uint16_t currencyType_, uint32_t userPk_, uint32_t itemPrice_) {
    semaphore.acquire();

    MYSQL* ConnPtr = GetConnection();
    if (!ConnPtr) {
        std::cerr << "[BuyItem] dbPool is empty. Failed to get DB connection." << '\n';
        return false;
    }

    auto tempAutoConn = AutoConn(ConnPtr, dbPool, dbPoolMutex, semaphore);
    
    MYSQL_RES* Result;
    MYSQL_ROW Row;
    
    mysql_autocommit(ConnPtr, false); // Transaction start

    // 금액 처리
    MYSQL_STMT* goldStmt = mysql_stmt_init(ConnPtr);
    std::string query;

    if (currencyType_ == 0) { // 구매 수단이 골드일때
        query = "UPDATE USERS SET gold = gold - ? WHERE id = ?";
    }
    else if (currencyType_ == 1) { // 구매 수단이 Cash일때
        query = "UPDATE USERS SET cash = cash - ? WHERE id = ?";
    }
    else if (currencyType_ == 2) { // 구매 수단이 마일리지일때
        query = "UPDATE USERS SET mileage = mileage - ? WHERE id = ?";
    }
    else {
        std::cerr << "[BuyItem] Unknown currencyType : " << itemType_ << '\n';
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    if (mysql_stmt_prepare(goldStmt, query.c_str(), query.length()) != 0) {
        std::cerr << "[BuyItem] Statement prepare error : " << mysql_stmt_error(goldStmt) << std::endl;
        return false;
    }

    MYSQL_BIND goldBind[2] = {};
    goldBind[0].buffer_type = MYSQL_TYPE_LONG;
    goldBind[0].buffer = (char*)&itemPrice_;
    goldBind[0].is_unsigned = true;

    goldBind[1].buffer_type = MYSQL_TYPE_LONG;
    goldBind[1].buffer = (char*)&userPk_;
    goldBind[1].is_unsigned = true;

    if (mysql_stmt_bind_param(goldStmt, goldBind) != 0 || mysql_stmt_execute(goldStmt) != 0 || mysql_stmt_affected_rows(goldStmt) == 0) {
        mysql_stmt_close(goldStmt);
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    mysql_stmt_close(goldStmt);

    // 인벤토리 처리
    MYSQL_STMT* invenStmt = mysql_stmt_init(ConnPtr);

    if (itemType_ == 0) { // 처리해야 할 아이템이 장비 아이템일 때
        query = "INSERT INTO Equipment(user_pk, item_code, daysOrCount) value(?,?,?)";
    }
    else if (itemType_ == 1) { // 처리해야 할 아이템이 소비 아이템일 때
        query = "INSERT INTO Consumables(user_pk, item_code, daysOrCount) value(?,?,?)";
    }
    else if (itemType_ == 2) { // 처리해야 할 아이템이 재료 아이템일 때
        query = "INSERT INTO Materials(user_pk, item_code, daysOrCount) value(?,?,?)";
    }
    else {
        std::cerr << "[BuyItem] Unknown itemType : " << itemType_ << '\n';
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    if (mysql_stmt_prepare(invenStmt, query.c_str(), query.length()) != 0) {
        std::cerr << "[BuyItem] Statement prepare error : " << mysql_stmt_error(invenStmt) << std::endl;
        mysql_stmt_close(invenStmt);
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    MYSQL_BIND invenBind[3] = {};
    invenBind[0].buffer_type = MYSQL_TYPE_LONG;
    invenBind[0].buffer = (char*)&userPk_;
    invenBind[0].is_unsigned = true;

    invenBind[1].buffer_type = MYSQL_TYPE_LONG;
    invenBind[1].buffer = (char*)&itemCode;
    invenBind[1].is_unsigned = true;

    invenBind[2].buffer_type = MYSQL_TYPE_LONG;
    invenBind[2].buffer = (char*)&daysOrCounts_;
    invenBind[2].is_unsigned = true;

    if (mysql_stmt_bind_param(invenStmt, invenBind) != 0 || mysql_stmt_execute(invenStmt) != 0 || mysql_stmt_affected_rows(invenStmt) == 0) {
        mysql_stmt_close(invenStmt);
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    mysql_stmt_close(invenStmt); 

    if (mysql_commit(ConnPtr) != 0) {
        std::cerr << "[BuyItem] Commit failed" << '\n';
        mysql_rollback(ConnPtr);
        mysql_autocommit(ConnPtr, true);
        return false;
    }

    mysql_autocommit(ConnPtr, true);
    return true;
}