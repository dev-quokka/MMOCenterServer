#include "PassRewardManager.h"

PassRewardManager& PassRewardManager::GetInstance() {
	static PassRewardManager instance;
	return instance;
}

// Mysql���� ������ �ε� �� ����
bool PassRewardManager::LoadFromMySQL(std::string passId_, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_, std::vector<uint16_t>& passExpLimit_) {

	if (passMap.count(passId_)) { // �̹� �����Ͱ� �ε�Ǿ����Ƿ� �ߺ� ȣ�� ����
		std::cout << "[PassRewardManager::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}   

	PassRewardData passRewardData;
	passRewardData.LoadFromMySQL(PassDataMap_, passExpLimit_);

	passMap[passId_] = passRewardData;

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