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
    uint16_t days = 0; // ��� ����
    CurrencyType currencyType; // ��������
    const ConsumableItemData* itemInfo = nullptr; // ������ ����
};

struct ShopMaterialItem {
    uint32_t itemPrice = 0;
    uint16_t itemCode = 0;
    uint16_t days = 0; // ��� ����
    CurrencyType currencyType; // ��������
    const MaterialItemData* itemInfo = nullptr; // ������ ����
};