#pragma once
#pragma comment (lib, "libmysql.lib")

#include <iostream>
#include <cstdint>
#include <string>
#include <mysql.h>
#include <sstream>
#include <vector>
#include <unordered_map>

#include "UserSyncData.h"
#include "ItemData.h"
#include "ShopItemData.h"

class MySQLManager {
public:
	~MySQLManager() {
		mysql_close(ConnPtr);
		std::cout << "MySQL End" << std::endl;
	}

	// ====================== INITIALIZATION =======================
	bool init();
	std::unordered_map<uint16_t, EquipmentItemData> GetEquipmentItemData();
	std::unordered_map<uint16_t, ConsumableItemData> GetConsumableItemData();
	std::unordered_map<uint16_t, MaterialItemData> GetMaterialItemData();
	std::unordered_map<std::pair<uint16_t, uint16_t>, ShopEquipmentItem> GetShopEquipmentItem();
	std::unordered_map<uint16_t, ShopConsumableItem> GetShopConsumableItem();
	std::unordered_map<uint16_t, ShopMaterialItem> GetShopMaterialItem();


	// ======================= SYNCRONIZATION =======================
	bool LogoutSync(uint32_t userPk_, USERINFO userInfo_, std::vector<EQUIPMENT> userEquip_, std::vector<CONSUMABLES> userConsum_, std::vector<MATERIALS> userMat_);
	bool SyncUserInfo(uint32_t userPk_, USERINFO userInfo_);
	bool SyncEquipment(uint32_t userPk_, std::vector<EQUIPMENT> userEquip_);
	bool SyncConsumables(uint32_t userPk_, std::vector<CONSUMABLES> userConsum_);
	bool SyncMaterials(uint32_t userPk_, std::vector<MATERIALS> userMat_);
	bool MySQLSyncEqipmentEnhace(uint32_t userPk_, uint16_t itemPosition, uint16_t enhancement);
	bool MySQLSyncUserRaidScore(uint32_t userPk_, unsigned int userScore_, std::string userId_);

	bool CashCharge(uint32_t userPk_, uint32_t chargedAmount);
	bool BuyItem(uint16_t itemCode, uint16_t daysOrCounts_, uint16_t itemType_, uint16_t currencyType_, uint32_t userPk_, uint32_t itemPrice_);

private:
	MYSQL Conn;
	MYSQL* ConnPtr = NULL;
	MYSQL_RES* Result;
	MYSQL_ROW Row;

	int MysqlResult;
};