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

struct ShopEquipmentKeyHash { // ShopEquipmentKey�� �ؽ� �Լ� (unordered_map���� ���)
    size_t operator()(const ShopEquipmentKey& k) const noexcept {
        return std::hash<uint16_t>()(k.itemCode)^(std::hash<uint16_t>()(k.days) << 1);
    }
};

struct ShopEquipmentItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    uint16_t days = 0; // ��� ����
    CurrencyType currencyType; // ��������
    const EquipmentItemData* itemInfo = nullptr; // ������ ����
};

struct ShopConsumableItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    CurrencyType currencyType; // ��������
    const ConsumableItemData* itemInfo = nullptr; // ������ ����
};

struct ShopMaterialItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    CurrencyType currencyType; // ��������
    const MaterialItemData* itemInfo = nullptr; // ������ ����
};