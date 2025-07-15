#include "MySQLManager.h"

// ====================== INITIALIZATION =======================

bool MySQLManager::init() {
    mysql_init(&Conn);

    ConnPtr = mysql_real_connect(&Conn, "127.0.0.1", "quokka", "1234", "iocp", 3306, (char*)NULL, 0);
    if (ConnPtr == NULL) {
        std::cout << mysql_error(&Conn) << std::endl;
        std::cout << "Mysql Connection Fail" << std::endl;
        return false;
    }

    std::cout << "Mysql Connection Success" << std::endl;
    return true;
}

std::unordered_map<uint16_t, EquipmentItemData> MySQLManager::GetEquipmentItemData() {
    std::string query_s = "SELECT item_code, itemName, attackPower FROM EquipmentData";

    std::unordered_map<uint16_t, EquipmentItemData> tempEM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[GetEquipmentItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempEM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[GetEquipmentItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempEM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2]) continue;

            EquipmentItemData equipmentData;
            equipmentData.itemCode = (uint16_t)std::stoi(Row[0]);
            equipmentData.itemName = Row[1];
            equipmentData.attackPower = (uint16_t)std::stoi(Row[2]);

            tempEM[equipmentData.itemCode] = equipmentData;
        }

        mysql_free_result(Result);
        return tempEM;
    }
    catch (const std::exception& e) {
        std::cerr << "[GetEquipmentItemData] Exception Error : " << e.what() << std::endl;
        tempEM.clear();
        return tempEM;
    }
}

std::unordered_map<uint16_t, ConsumableItemData> MySQLManager::GetConsumableItemData() {
    std::string query_s = "SELECT item_code, itemName FROM ConsumableData";

    std::unordered_map<uint16_t, ConsumableItemData> tempCM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ConsumableItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempCM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ConsumableItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempCM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            ConsumableItemData consumableData;
            consumableData.itemCode = (uint16_t)std::stoi(Row[0]);
            consumableData.itemName = Row[1];

            tempCM[consumableData.itemCode] = consumableData;
        }

        mysql_free_result(Result);
        return tempCM;
    }
    catch (const std::exception& e) {
        std::cerr << "[ConsumableItemData] Exception Error : " << e.what() << std::endl;
        tempCM.clear();
        return tempCM;
    }
}

std::unordered_map<uint16_t, MaterialItemData> MySQLManager::GetMaterialItemData() {
    std::string query_s = "SELECT item_code, itemName FROM MaterialData";

    std::unordered_map<uint16_t, MaterialItemData> tempMM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[MaterialItemData] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempMM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[MaterialItemData] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempMM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1]) continue;

            MaterialItemData materialData;
            materialData.itemCode = (uint16_t)std::stoi(Row[0]);
            materialData.itemName = Row[1];

            tempMM[materialData.itemCode] = materialData;
        }

        mysql_free_result(Result);
        return tempMM;
    }
    catch (const std::exception& e) {
        std::cerr << "[MaterialItemData] Exception Error : " << e.what() << std::endl;
        tempMM.clear();
        return tempMM;
    }
}

std::unordered_map<uint16_t, ShopEquipmentItem> MySQLManager::GetShopEquipmentItem() {
    std::string query_s = "SELECT item_code, itemPrice, days, currencyType FROM ShopEquipmentData";

    std::unordered_map<uint16_t, ShopEquipmentItem> tempSEM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ShopEquipmentItem] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempSEM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ShopEquipmentItem] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempSEM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

            ShopEquipmentItem shopEquipmentData;
            shopEquipmentData.itemCode = (uint16_t)std::stoi(Row[0]);
            shopEquipmentData.itemPrice = static_cast<uint32_t>(std::stoul(Row[1]));
            shopEquipmentData.days = (uint16_t)std::stoi(Row[2]);
            shopEquipmentData.currencyType = static_cast<CurrencyType>(std::stoi(Row[3]));

            tempSEM[shopEquipmentData.itemCode] = shopEquipmentData;
        }

        mysql_free_result(Result);
        return tempSEM;
    }
    catch (const std::exception& e) {
        std::cerr << "[ShopEquipmentItem] Exception Error : " << e.what() << std::endl;
        tempSEM.clear();
        return tempSEM;
    }
}

std::unordered_map<uint16_t, ShopConsumableItem> MySQLManager::GetShopConsumableItem() {
    std::string query_s = "SELECT item_code, itemPrice, days, currencyType FROM ShopConsumableData";

    std::unordered_map<uint16_t, ShopConsumableItem> tempSCM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ShopConsumableItem] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempSCM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ShopConsumableItem] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempSCM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

            ShopConsumableItem ShopConsumableData;
            ShopConsumableData.itemCode = (uint16_t)std::stoi(Row[0]);
            ShopConsumableData.itemPrice = static_cast<uint32_t>(std::stoul(Row[1]));
            ShopConsumableData.days = (uint16_t)std::stoi(Row[2]);
            ShopConsumableData.currencyType = static_cast<CurrencyType>(std::stoi(Row[3]));

            tempSCM[ShopConsumableData.itemCode] = ShopConsumableData;
        }

        mysql_free_result(Result);
        return tempSCM;
    }
    catch (const std::exception& e) {
        std::cerr << "[ShopConsumableItem] Exception Error : " << e.what() << std::endl;
        tempSCM.clear();
        return tempSCM;
    }
}

std::unordered_map<uint16_t, ShopMaterialItem> MySQLManager::GetShopMaterialItem() {
    std::string query_s = "SELECT item_code, itemPrice, days, currencyType FROM ShopMaterialData";

    std::unordered_map<uint16_t, ShopMaterialItem> tempSMM;

    const char* Query = query_s.c_str();

    if (mysql_query(ConnPtr, Query) != 0) {
        std::cerr << "[ShopMaterialItem] Query Failed : " << mysql_error(ConnPtr) << std::endl;
        return tempSMM;
    }

    try {
        Result = mysql_store_result(ConnPtr);
        if (Result == nullptr) {
            std::cerr << "[ShopMaterialItem] Failed to store result : " << mysql_error(ConnPtr) << std::endl;
            return tempSMM;
        }

        while ((Row = mysql_fetch_row(Result)) != NULL) {
            if (!Row[0] || !Row[1] || !Row[2] || !Row[3]) continue;

            ShopMaterialItem ShopMaterialData;
            ShopMaterialData.itemCode = (uint16_t)std::stoi(Row[0]);
            ShopMaterialData.itemPrice = static_cast<uint32_t>(std::stoul(Row[1]));
            ShopMaterialData.days = (uint16_t)std::stoi(Row[2]);
            ShopMaterialData.currencyType = static_cast<CurrencyType>(std::stoi(Row[3]));

            tempSMM[ShopMaterialData.itemCode] = ShopMaterialData;
        }

        mysql_free_result(Result);
        return tempSMM;
    }
    catch (const std::exception& e) {
        std::cerr << "[ShopMaterialItem] Exception Error : " << e.what() << std::endl;
        tempSMM.clear();
        return tempSMM;
    }
}


// ======================= SYNCRONIZATION =======================

bool MySQLManager::LogoutSync(uint16_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, 
    std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_) {
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

bool MySQLManager::SyncUserInfo(uint16_t userPk_, USERINFO userInfo_) {
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

bool MySQLManager::SyncEquipment(uint16_t userPk_, std::vector<EQUIPMENT> userEquip_) {    
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

bool MySQLManager::SyncConsumables(uint16_t userPk_, std::vector<CONSUMABLES> userConsum_) {
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Consumables SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "count = CASE ";

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

bool MySQLManager::SyncMaterials(uint16_t userPk_, std::vector<MATERIALS> userMat_) {
    try {
        std::ostringstream query_s;
        query_s << "UPDATE Materials SET ";

        std::ostringstream item_code_case, count_case, where;
        item_code_case << "Item_code = CASE ";
        count_case << "count = CASE ";

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

bool MySQLManager::MySQLSyncEqipmentEnhace(uint16_t userPk_, uint16_t itemPosition_, uint16_t enhancement_) {
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

bool MySQLManager::MySQLSyncUserRaidScore(uint16_t userPk_, unsigned int userScore_, std::string userId_) {
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

bool MySQLManager::CashCharge(uint16_t userPk_, uint32_t chargedAmount) {
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