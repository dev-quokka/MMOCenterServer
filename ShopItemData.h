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