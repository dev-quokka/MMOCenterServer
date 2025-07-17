#include "ItemDataManager.h"

ItemDataManager& ItemDataManager::GetInstance() {
	static ItemDataManager instance;
	return instance;
}

bool ItemDataManager::LoadFromMySQL(std::unordered_map<uint16_t, std::unique_ptr<ItemData>>& em){
	
	if (loadCheck) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[ItemDataManager] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	ItemMap = std::move(em);

	loadCheck = true;
	return true;
}

const ItemData* ItemDataManager::GetItemData(uint16_t itemId) const {
	auto it = ItemMap.find(itemId);
	if (it == ItemMap.end()) {
		return nullptr;
	}

	return it->second.get();
}
