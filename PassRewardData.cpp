#include "PassRewardData.h"

bool PassRewardData::LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, PassDataForSend, PassDataKeyHash>& PassDataMap_) {
	
	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[PassRewardData::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}
	
	passDataMap = std::move(PassDataMap_);

	passInfo = passInfo_;
	loadCheck = true;
	return true;
}

const PassDataForSend* PassRewardData::GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const {
	auto it = passDataMap.find({ passLevel_ , passCurrencyType_ });
	if (it == passDataMap.end()) {
		return nullptr;
	}

	return &it->second;
}

const uint16_t PassRewardData::GetPassMaxLevel(uint16_t passLevel_) const {
	return passInfo.passMaxLevel;
}