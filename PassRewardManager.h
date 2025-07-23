#pragma once

#include "PassRewardData.h"

class PassRewardManager { // Singleton class for managing item data
public:
    static PassRewardManager& GetInstance();

    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(std::vector<std::pair<std::string, PassInfo>> passIdVector_,
        std::unordered_map<std::string, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>>& passDataMap_,
        std::vector<uint32_t>& passExpLimit_);

    const PassData* GetPassItemDataByPassId(std::string& passId_, uint16_t passLevel_, uint16_t passCurrencyType_) const;
    const uint32_t GetPassLevelUpExp(std::string& passId_, uint16_t passLevel_) const;

private:
    PassRewardManager() = default;
    PassRewardManager(const PassRewardManager&) = delete;
    PassRewardManager& operator=(const PassRewardManager&) = delete;
    PassRewardManager(PassRewardManager&&) = delete;
    PassRewardManager& operator=(PassRewardManager&&) = delete;

    std::unordered_map<std::string, PassRewardData> passMap;
    std::vector<uint32_t> passExpLimit; // �н� ���� �� �ʿ� ����ġ��

    bool loadCheck = false;
};