#pragma once

#include "PassData.h"
#include "PassDataPacket.h"

class PassRewardData {
public:
    // Mysql에서 데이터 로드 후 세팅
    bool LoadFromMySQL(PassInfo& passInfo_, std::unordered_map<PassDataKey, PassDataForSend, PassDataKeyHash>& PassDataMap_);

    const PassDataForSend* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;

    const uint16_t GetPassMaxLevel(uint16_t passLevel_) const;

private:
    std::unordered_map<PassDataKey, PassDataForSend, PassDataKeyHash> passDataMap;

    PassInfo passInfo;

    bool loadCheck = false;
};