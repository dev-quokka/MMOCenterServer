#include "PassRewardData.h"

bool PassRewardData::LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_) {
	
	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
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
			std::cout << "패스 레벨 : " << passItem->passLevel << ", 패스 아이템 결제 타입 : FREE" << '\n';
			break;
		case PassCurrencyType::CASH1:
			std::cout << "패스 레벨 : " << passItem->passLevel << ", 패스 아이템 결제 타입 : CASH1" << '\n';
			break;
		default:
			std::cout << "패스 레벨 : " << passItem->passLevel << ", 패스 아이템 결제 타입 : UNKNOWN" << '\n';
			break;
		}

		passItem.get()->itemInfo = tempItemInfo;
	}

	passInfo = passInfo_;
	std::cout << "이벤트 시작 : " << passInfo.eventStart << '\n';
	std::cout << "이벤트 종료 : " << passInfo.eventEnd << '\n';
	std::cout << "패스 최대 레벨 : " << passInfo.passMaxLevel << '\n';
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