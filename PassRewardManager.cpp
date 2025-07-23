#include "PassRewardManager.h"

PassRewardManager& PassRewardManager::GetInstance() {
	static PassRewardManager instance;
	return instance;
}

// Mysql에서 데이터 로드 후 세팅
bool PassRewardManager::LoadFromMySQL(std::vector<std::pair<std::string, PassInfo>> passIdVector_, std::unordered_map<std::string, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>>& passDataMap_, std::vector<uint32_t>& passExpLimit_) {

	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[PassRewardManager::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}

	for (auto& t : passIdVector_) {
		PassRewardData passRewardData;
		passRewardData.LoadFromMySQL(t.second, passDataMap_[t.first]);

		passMap[t.first] = std::move(passRewardData);
	}

	passExpLimit = std::move(passExpLimit_);
	
	loadCheck = true;
	return true;
}

const PassData* PassRewardManager::GetPassItemDataByPassId(std::string& passId_, uint16_t passLevel_, uint16_t passCurrencyType_) const {
	auto passIter = passMap.find(passId_);
	if (passIter != passMap.end()) {
		auto* passData = passIter->second.GetPassItemData(passLevel_, passCurrencyType_);
		if (!passData) {
			std::cout << "[PassRewardManager::GetPassItemDataByPassId] Get PassData Failed. passId : " << passId_ 
				<< ", level : " << passLevel_
				<< ", currencyType : " << passCurrencyType_ << '\n';
			return nullptr;
		}
		return passData;
	}

	return nullptr;
}

const uint32_t PassRewardManager::GetPassLevelUpExp(std::string& passId_, uint16_t passLevel_) const {
	auto passIter = passMap.find(passId_);
	if (passIter != passMap.end()) {
		auto tempPassMaxLevel = passIter->second.GetPassMaxLevel(passLevel_);
		if (passLevel_ > tempPassMaxLevel) return 0;
		return passExpLimit[passLevel_];
	}
	return 0;
};