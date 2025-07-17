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

struct ItemDataKey {
    uint16_t itemCode = 0;
    uint16_t ItemType = 0;

    ItemDataKey(uint16_t itemCode_, uint16_t ItemType_) : itemCode(itemCode_), ItemType(ItemType_) {}

    bool operator==(const ItemDataKey& other) const {
        return itemCode == other.itemCode && ItemType == other.ItemType;
    }
};

struct ItemDataKeyHash { // ItemDataKey�� �ؽ� �Լ� (unordered_map���� ���)
    size_t operator()(const ItemDataKey& k) const noexcept {
        return std::hash<uint16_t>()(k.itemCode) ^ (std::hash<uint16_t>()(k.ItemType) << 1);
    }
};