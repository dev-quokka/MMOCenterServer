#include "ItemDataManager.h"

ItemDataManager& ItemDataManager::GetInstance() {
	static ItemDataManager instance;
	return instance;
}

bool ItemDataManager::LoadFromMySQL(std::unordered_map<uint16_t, EquipmentItemData>& em, std::unordered_map<uint16_t, ConsumableItemData>& cm, std::unordered_map<uint16_t, MaterialItemData>& mm){
	
	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[ItemDataManager] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	equipmentItemMap = std::move(em);
	consumableItemMap = std::move(cm);
	materialItemMap = std::move(mm);
	
	loadCheck = true;
	return true;
}

const EquipmentItemData* ItemDataManager::GetEquipment(uint16_t itemId) const {
	auto it = equipmentItemMap.find(itemId);
	if (it == equipmentItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const ConsumableItemData* ItemDataManager::GetConsumable(uint16_t itemId) const {
	auto it = consumableItemMap.find(itemId);
	if (it == consumableItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}

const MaterialItemData* ItemDataManager::GetMaterial(uint16_t itemId) const {
	auto it = materialItemMap.find(itemId);
	if (it == materialItemMap.end()) {
		return nullptr;
	}

	return &(it->second);
}
