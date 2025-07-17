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

    // ���� �ʿ��� ������(���� ���� �� ��� ����) �߰� ����
};

struct ConsumableItemData : public ItemData {

    // ���� �ʿ��� ������ �߰� ����
};

struct MaterialItemData : public ItemData {

    // ���� �ʿ��� ������ �߰� ����
};