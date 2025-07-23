#include "PassRewardData.h"

bool PassRewardData::LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_) {
	
	if (loadCheck) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[PassRewardData::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	passDataMap = std::move(PassDataMap_);
	for (auto& [passKey, passItem] : passDataMap) {
		auto tempItemInfo = ItemDataManager::GetInstance().GetItemData(passItem.get()->itemCode, static_cast<uint16_t>(passItem.get()->itemType));
		if (!tempItemInfo) {
			std::cerr << "[PassRewardData::LoadFromMySQL] Invalid itemCode: " << passItem.get()->itemCode << '\n';
			continue;
		}
		
		switch (passItem->passCurrencyType) {
		case PassCurrencyType::FREE:
			std::cout << "�н� ���� : " << passItem->passLevel << ", �н� ������ ���� Ÿ�� : FREE" << '\n';
			break;
		case PassCurrencyType::CASH1:
			std::cout << "�н� ���� : " << passItem->passLevel << ", �н� ������ ���� Ÿ�� : CASH1" << '\n';
			break;
		default:
			std::cout << "�н� ���� : " << passItem->passLevel << ", �н� ������ ���� Ÿ�� : UNKNOWN" << '\n';
			break;
		}

		passItem.get()->itemInfo = tempItemInfo;
	}

	passInfo = passInfo_;
	std::cout << "�̺�Ʈ ���� : " << passInfo.eventStart << '\n';
	std::cout << "�̺�Ʈ ���� : " << passInfo.eventEnd << '\n';
	std::cout << "�н� �ִ� ���� : " << passInfo.passMaxLevel << '\n';
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

const uint16_t PassRewardData::GetPassMaxLevel(uint16_t passLevel_) const {
	return passInfo.passMaxLevel;
}