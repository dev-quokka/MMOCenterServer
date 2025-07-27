#include "PassRewardManager.h"

PassRewardManager& PassRewardManager::GetInstance() {
	static PassRewardManager instance;
	return instance;
}

// Mysql에서 데이터 로드 후 세팅
bool PassRewardManager::LoadFromMySQL(std::vector<std::pair<std::string, PassInfo>> passIdVector_, std::unordered_map<std::string, std::unordered_map<PassDataKey, PassItemForSend, PassDataKeyHash>>& passDataMap_, std::vector<uint16_t>& passExpLimit_, char* packetBuffer_, PassItemForSend* passVector_, size_t packetSize_) {

	if (loadCheck) { // 이미 데이터가 로드되었으므로 중복 호출 방지
		std::cout << "[PassRewardManager::LoadFromMySQL] LoadFromMySQL already completed." << '\n';
		return true;
	}

	std::vector<PassItemForSend> passDataVector;

	for (auto& pdm : passDataMap_) {
		const std::string& tempPassId = pdm.first;

		for (auto& [passKey, passItem] : pdm.second) {
			auto tempItemInfo = ItemDataManager::GetInstance().GetItemData(passItem.itemCode, passItem.itemType);
			if (!tempItemInfo) {
				std::cerr << "[PassRewardData::LoadFromMySQL] Invalid itemCode: " << passItem.itemCode << '\n';
				continue;
			}

			strncpy_s(passItem.itemName, ItemDataManager::GetInstance().GetItemData(passItem.itemCode, passItem.itemType)->itemName.c_str(), MAX_ITEM_ID_LEN);
			strncpy_s(passItem.passId, tempPassId.c_str(), MAX_ITEM_ID_LEN);
			passDataVector.emplace_back(passItem);
		}
	}

	std::sort(passDataVector.begin(), passDataVector.end(), [](const PassItemForSend& a, const PassItemForSend& b) { // passId 기준 정렬 후, 레벨과 결제 타입별 오름 차순 정리
		std::string tempAPassId = a.passId;
		std::string tempBPassId = b.passId;

		return std::tie(tempAPassId, a.passLevel, a.passCurrencyType) <
			   std::tie(tempBPassId, b.passLevel, b.passCurrencyType);
	});

	for (auto& t : passIdVector_) {
		PassRewardData passRewardData;
		passRewardData.LoadFromMySQL(t.second, passDataMap_[t.first]);

		passMap[t.first] = std::move(passRewardData);
	}

	passDataForSend.passPacketBuffer = std::move(packetBuffer_);

	for (int i = 0; i < passDataVector.size(); ++i) {
		passVector_[i] = passDataVector[i];
	}

	passDataForSend.passPacketSize = packetSize_;

	passExpLimit = std::move(passExpLimit_);

	loadCheck = true;
	return true;
}

const PassDataForSend& PassRewardManager::GetPassData() const{
	return passDataForSend;
}

const PassItemForSend* PassRewardManager::GetPassItemDataByPassId(std::string& passId_, uint16_t passLevel_, uint16_t passCurrencyType_) const {
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

const std::pair<uint16_t, uint16_t> PassRewardManager::PassExpUp(uint16_t acqPassExp_, uint16_t userLevel, uint16_t currentPassExp_) {
	currentPassExp_ += acqPassExp_;

	uint16_t levelUpCount = 0;

	while(passExpLimit[userLevel + levelUpCount] <= currentPassExp_) { // 레벨 업 체크
		currentPassExp_ -= passExpLimit[userLevel + levelUpCount];
		levelUpCount++;
	}

	return { levelUpCount , currentPassExp_ }; // {레벨 증가량, 현재 경험치}	
}