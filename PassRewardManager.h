#pragma once

#include "PassRewardData.h"

class PassRewardManager { // Singleton class for managing item data
public:
    static PassRewardManager& GetInstance();

    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(std::vector<std::pair<std::string, PassInfo>> passIdVector_,
        std::unordered_map<std::string, std::unordered_map<PassDataKey, PassDataForSend, PassDataKeyHash>>& passDataMap_,
        std::vector<uint16_t>& passExpLimit_);

    // ���� ���� �� ��ü �н� ������ �����ϱ� ���� ���� ��ȯ
    const std::vector<PassDataForSend>& GetPassData() const;

    const PassDataForSend* GetPassItemDataByPassId(std::string& passId_, uint16_t passLevel_, uint16_t passCurrencyType_) const;
    // const uint32_t GetPassLevelUpExp(std::string& passId_, uint16_t passLevel_) const;

    const std::pair<uint16_t, uint16_t> PassExpUp(uint16_t acqPassExp_, uint16_t userLevel, uint16_t currentPassExp_);

private:
    PassRewardManager() = default;
    PassRewardManager(const PassRewardManager&) = delete;
    PassRewardManager& operator=(const PassRewardManager&) = delete;
    PassRewardManager(PassRewardManager&&) = delete;
    PassRewardManager& operator=(PassRewardManager&&) = delete;

    std::unordered_map<std::string, PassRewardData> passMap;
    std::vector<PassDataForSend> passDataVector; // �� �н� ������ ������ ������
    std::vector<uint16_t> passExpLimit; // �н� ���� �� �ʿ� ����ġ��

    bool loadCheck = false;
};