#pragma once
#include "ShopItemData.h"
#include "ItemDataManager.h"

class ShopDataManager { // Singleton class for managing shop data
public:
	static ShopDataManager& GetInstance();

	// MySQL���� ���� �����͸� �ε��� ��, �� ������ �ڵ忡 �ش��ϴ� ItemData ������ ��Ī�Ͽ� ����
	bool LoadFromMySQL(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& shopItemMap_);

	// ������ �������� Ư�� �������� �������� �� �ش� ������ ������ ��ȯ
	const ShopItem* GetItem(uint16_t itemId, uint16_t days) const;

	// ���� ���� �� ��ü ���� ������ �����ϱ� ���� ���� ��ȯ
	const std::vector<ShopItem>& GetShopData() const;

private:
	ShopDataManager() = default;
	ShopDataManager(const ShopDataManager&) = delete;
	ShopDataManager& operator=(const ShopDataManager&) = delete;
	ShopDataManager(ShopDataManager&&) = delete;
	ShopDataManager& operator=(ShopDataManager&&) = delete;

	std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash> shopItemMap;
	std::vector<ShopItem> shopItemVector;

	bool loadCheck = false;
};