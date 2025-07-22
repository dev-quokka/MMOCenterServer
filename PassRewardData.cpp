#include "PassRewardData.h"

bool PassRewardData::LoadFromMySQL(std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_, std::vector<uint16_t>& passExpLimit_) {
	
	if (loadCheck) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[PassRewardData::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	passDataMap = std::move(PassDataMap_);
	for (auto& [passKey, passItem] : passDataMap) {
		auto tempItemInfo = ItemDataManager::GetInstance().GetItemData(passItem.get()->itemCode, static_cast<uint16_t>(passItem.get()->itemType));
		if (!tempItemInfo) {
			std::cerr << "[ShopDataManager::LoadFromMySQL] Invalid itemCode: " << passItem.get()->itemCode << '\n';
			continue;
		}

		passItem.get()->itemInfo = tempItemInfo;
	}

	passExpLimit = std::move(passExpLimit_);

	loadCheck = true;
	return true;
}

const PassData* PassRewardData::GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const {
	auto it = passDataMap.find({ passLevel_ , passCurrencyType_ });
	if (it == passDataMap.end()) {
		return nullptr;
	}

	return it->second.get();
}