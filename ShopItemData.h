#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include <algorithm>

#include "ItemDataManager.h"

enum class CurrencyType : uint16_t {
    GOLD,
    CASH,
    MILEAGE
};

inline const std::unordered_map<CurrencyType, std::string> currencyTypeMap = {
     {CurrencyType::GOLD, "gold"},
     {CurrencyType::CASH, "cash"},
     {CurrencyType::MILEAGE, "mileage"},
};

struct ShopEquipmentKey {
    uint16_t itemCode = 0;
    uint16_t days = 0;
  
    ShopEquipmentKey(uint16_t itemCode_, uint16_t days_) : itemCode(itemCode_), days(days_) {}

    bool operator==(const ShopEquipmentKey& other) const {
        return itemCode == other.itemCode && days == other.days;
    }
};

struct ShopEquipmentKeyHash { // ShopEquipmentKey용 해시 함수 (unordered_map에서 사용)
    size_t operator()(const ShopEquipmentKey& k) const noexcept {
        return std::hash<uint16_t>()(k.itemCode)^(std::hash<uint16_t>()(k.days) << 1);
    }
};

struct ShopEquipmentItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    uint16_t days = 0; // 사용 기한
    CurrencyType currencyType; // 결제수단
    const EquipmentItemData* itemInfo = nullptr; // 아이템 정보
};

struct ShopConsumableItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    CurrencyType currencyType; // 결제수단
    const ConsumableItemData* itemInfo = nullptr; // 아이템 정보
};

struct ShopMaterialItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    CurrencyType currencyType; // 결제수단
    const MaterialItemData* itemInfo = nullptr; // 아이템 정보
};