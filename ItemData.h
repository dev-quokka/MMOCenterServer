#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

struct EquipmentItemData {
    std::string itemName;
    uint16_t itemCode;
    uint16_t attackPower;

    // 추후 필요한 데이터(레벨 제한 및 등급 제한) 추가 예정
};

struct ConsumableItemData {
    std::string itemName;
    uint16_t itemCode;

    // 추후 필요한 데이터 추가 예정
};

struct MaterialItemData {
    std::string itemName;
    uint16_t itemCode;

    // 추후 필요한 데이터 추가 예정
};