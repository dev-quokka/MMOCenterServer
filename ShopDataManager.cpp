#include "ShopDataManager.h"

ShopDataManager& ShopDataManager::GetInstance() {
    static ShopDataManager instance;
    return instance;
}

bool ShopDataManager::LoadFromMySQL(std::unordered_map<std::pair<uint16_t, uint16_t>, ShopEquipmentItem>& em, std::unordered_map<uint16_t, ShopConsumableItem>& cm, std::unordered_map<uint16_t, ShopMaterialItem>& mm) {
	
	if (loadCheck) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[ShopDataManager] LoadFromMySQL already completed." << '\n';
		return true;
	}

	// ���� ��� ������ ����
	shopEquipmentItemMap = std::move(em);
	for (auto& [itemId, shopItem] : shopEquipmentItemMap) {
		shopItem.itemInfo = ItemDataManager::GetInstance().GetEquipment(shopItem.itemCode);
		shopEquipmentItemVector.emplace_back(shopItem);
	}

	std::sort(shopEquipmentItemVector.begin(), shopEquipmentItemVector.end(), [](const auto& a, const auto& b) {
		return a.itemCode < b.itemCode; // ������ �ڵ� �� ����
	});


	// ���� �Һ� ������ ����
	shopConsumableItemMap = std::move(cm);
	for (auto& [itemId, shopItem] : shopConsumableItemMap) {
		shopItem.itemInfo = ItemDataManager::GetInstance().GetConsumable(shopItem.itemCode);
		shopConsumableItemVector.emplace_back(shopItem);
	}

	std::sort(shopConsumableItemVector.begin(), shopConsumableItemVector.end(), [](const auto& a, const auto& b) {
		return a.itemCode < b.itemCode; // ������ �ڵ� �� ����
	});


	// ���� ��� ������ ����
	shopMaterialItemMap = std::move(mm);
	for (auto& [itemId, shopItem] : shopMaterialItemMap) {
		shopItem.itemInfo = ItemDataManager::GetInstance().GetMaterial(shopItem.itemCode);
		shopMaterialItemVector.emplace_back(shopItem);
	}

	std::sort(shopMaterialItemVector.begin(), shopMaterialItemVector.end(), [](const auto& a, const auto& b) {
		return a.itemCode < b.itemCode; // ������ �ڵ� �� ����
	});


	loadCheck = true;
	return true;
}


const ShopEquipmentItem* ShopDataManager::GetEquipment(uint16_t itemId, uint16_t days) const {
	auto it = shopEquipmentItemMap.find({ itemId , days});
	if (it == shopEquipmentItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const ShopConsumableItem* ShopDataManager::GetConsumable(uint16_t itemId) const {
	auto it = shopConsumableItemMap.find(itemId);
	if (it == shopConsumableItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const ShopMaterialItem* ShopDataManager::GetMaterial(uint16_t itemId) const {
	auto it = shopMaterialItemMap.find(itemId);
	if (it == shopMaterialItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}


const std::vector<ShopEquipmentItem>& ShopDataManager::GetEquipmentVector() const {
	return shopEquipmentItemVector;
}

const std::vector<ShopConsumableItem>& ShopDataManager::GetConsumableVector() const {
	return shopConsumableItemVector;
}

const std::vector<ShopMaterialItem>& ShopDataManager::GetMaterialVector() const {
	return shopMaterialItemVector;
}