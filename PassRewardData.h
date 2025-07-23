#pragma once

#include "PassData.h"

class PassRewardData {
public:
    // Mysql에서 데이터 로드 후 세팅
    bool LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_);

    const PassData* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;
    const uint16_t GetPassMaxLevel(uint16_t passLevel_) const;

private:
    std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash> passDataMap; // { {패스 레벨, 패스 결제 타입}, 패스 아이템 데이터}

    PassInfo passInfo;

    bool loadCheck = false;
};