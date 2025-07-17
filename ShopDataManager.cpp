#include "ShopDataManager.h"

ShopDataManager& ShopDataManager::GetInstance() {
    static ShopDataManager instance;
    return instance;
}

bool ShopDataManager::LoadFromMySQL(std::unordered_map<ShopItemKey, ShopItem, ShopItemKeyHash>& em) {
	
	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[ShopDataManager] LoadFromMySQL already completed." << '\n';
		return true;
	}

	// 상점 장비 데이터 저장
	shopItemMap = std::move(em);
	for (auto& [itemId, shopItem] : shopItemMap) {
		
		auto tempItemInfo = ItemDataManager::GetInstance().GetItemData(shopItem.itemCode);
		if (!tempItemInfo) {
			std::cerr << "[LoadFromMySQL] Invalid itemCode: " << shopItem.itemCode << '\n';
			continue;
		}

		shopItem.itemInfo = tempItemInfo;
		shopItem.itemType = tempItemInfo->itemType;

		shopItemVector.emplace_back(shopItem);
	}

	std::sort(shopItemVector.begin(), shopItemVector.end(), [](const auto& a, const auto& b) { // itemType 기준 오름차순 정렬 후, itemCode 기준 오름차순 정렬
		if (a.itemType == b.itemType) return a.itemCode < b.itemCode;
		return a.itemType < b.itemType;
	});

	loadCheck = true;
	return true;
}

const ShopItem* ShopDataManager::GetItem(uint16_t itemId, uint16_t days) const {
	auto it = shopItemMap.find({ itemId , days});
	if (it == shopItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const std::vector<ShopItem>& ShopDataManager::GetShopData() const {
	return shopItemVector;
}