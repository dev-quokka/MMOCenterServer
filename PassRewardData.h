#pragma once

#include "PassData.h"

class PassRewardData {
public:
    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_);

    const PassData* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;
    const uint16_t GetPassMaxLevel(uint16_t passLevel_) const;

private:
    std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash> passDataMap; // { {�н� ����, �н� ���� Ÿ��}, �н� ������ ������}

    PassInfo passInfo;

    bool loadCheck = false;
};