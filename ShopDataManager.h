#pragma once

#include "ShopItemData.h"
#include "ItemDataManager.h"

class ShopDataManager { // Singleton class for managing shop data
public:
	static ShopDataManager& GetInstance();

	// MySQL에서 상점 데이터를 로드한 뒤, 각 아이템 코드에 해당하는 ItemData 정보를 매칭하여 저장
	bool LoadFromMySQL(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemMap_);

	// 유저가 상점에서 특정 아이템을 선택했을 때 해당 아이템 정보를 반환
	const ShopItemForSend* GetItem(uint16_t itemId, uint16_t days) const;

	// 유저 접속 시 전체 상점 정보를 전달하기 위한 벡터 반환
	const std::vector<ShopItemForSend>& GetShopData() const;

private:
	ShopDataManager() = default;
	ShopDataManager(const ShopDataManager&) = delete;
	ShopDataManager& operator=(const ShopDataManager&) = delete;
	ShopDataManager(ShopDataManager&&) = delete;
	ShopDataManager& operator=(ShopDataManager&&) = delete;

	std::unordered_map<ShopItemKey, ShopItemForSend, ShopItemKeyHash> shopItemMap;
	std::vector<ShopItemForSend> shopItemVector;

	bool loadCheck = false;
};