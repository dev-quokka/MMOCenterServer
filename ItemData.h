#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

enum class ItemType : uint16_t {
    EQUIPMENT,
    CONSUMABLE,
    MATERIAL
};

struct ItemData {
    std::string itemName = "";
    uint16_t itemCode = 0;
    ItemType itemType;
};

struct EquipmentItemData : public ItemData {
    uint16_t attackPower = 0;

    // 추후 필요한 데이터(레벨 제한 및 등급 제한) 추가 예정
};

struct ConsumableItemData : public ItemData {

    // 추후 필요한 데이터 추가 예정
};

struct MaterialItemData : public ItemData {

    // 추후 필요한 데이터 추가 예정
};