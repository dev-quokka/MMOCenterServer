#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

#include "ItemDataManager.h"

enum class PassCurrencyType : uint16_t { // ���� �н� or ���� �н�
    FREE,
    CASH1
};

struct PassInfo {
    std::string eventStart;
    std::string eventEnd;
    uint16_t passMaxLevel = 0;
};

struct PassData {
    uint16_t itemCode = 0;
    uint16_t passLevel = 0;
    uint16_t itemCount = 1; // ������ ����
    uint16_t daysOrCount = 0;
    ItemType itemType;
    PassCurrencyType passCurrencyType;
    const ItemData* itemInfo = nullptr; // ������ ����
};

struct PassDataKey {
    uint16_t passLevel = 0;
    uint16_t passCurrencyType = 0;

    PassDataKey(uint16_t passLevel_, uint16_t passCurrencyType_) : passLevel(passLevel_), passCurrencyType(passCurrencyType_) {}

    bool operator==(const PassDataKey& other) const {
        return passLevel == other.passLevel && passCurrencyType == other.passCurrencyType;
    }
};

struct PassDataKeyHash { // PassDataKey�� �ؽ� �Լ� (unordered_map���� ���)
    size_t operator()(const PassDataKey& k) const noexcept {
        return std::hash<uint16_t>()(k.passLevel) ^ (std::hash<uint16_t>()(k.passCurrencyType) << 1);
    }
};