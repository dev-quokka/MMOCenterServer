#pragma once

#include "PassData.h"

class PassRewardData {
public:
    // Mysql에서 데이터 로드 후 세팅
    bool LoadFromMySQL(std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_, std::vector<uint16_t>& passExpLimit_);

    const PassData* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;

private:
    std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash> passDataMap; // { {패스 레벨, 패스 결제 타입}, 패스 아이템 데이터}
    std::vector<uint16_t> passExpLimit; // 각 패스 레벨 별 필요 경험치양

    bool loadCheck = false;
};