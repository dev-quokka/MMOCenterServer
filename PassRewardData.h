#pragma once

#include "PassData.h"

class PassRewardData {
public:
    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash>& PassDataMap_, std::vector<uint16_t>& passExpLimit_);

    const PassData* GetPassItemData(uint16_t passLevel_, uint16_t passCurrencyType_) const;

private:
    std::unordered_map<PassDataKey, std::unique_ptr<PassData>, PassDataKeyHash> passDataMap; // { {�н� ����, �н� ���� Ÿ��}, �н� ������ ������}
    std::vector<uint16_t> passExpLimit; // �� �н� ���� �� �ʿ� ����ġ��

    bool loadCheck = false;
};