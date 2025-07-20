#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

#include "ShopItemPacket.h"

enum class ItemType : uint16_t {
    EQUIPMENT,
    CONSUMABLE,
    MATERIAL
};

struct ItemData {
    std::string itemName = "";
    uint16_t itemCode = 0;
    ItemType itemType;

    virtual void FillShopItemData(ShopItemForSend& shopItemData_) const {}
    virtual ~ItemData() {}
};

struct EquipmentItemData : public ItemData {
    uint16_t attackPower = 0;

    // ���� �ʿ��� ������(���� ���� �� ��� ����) �߰� ����

    void FillShopItemData(ShopItemForSend& shopItemData_) const override{
        shopItemData_.attackPower = attackPower;
    }
};

struct ConsumableItemData : public ItemData {

    // ���� �ʿ��� ������ �߰� ����

    void FillShopItemData(ShopItemForSend& shopItemData_) const override {

    }
};

struct MaterialItemData : public ItemData {

    // ���� �ʿ��� ������ �߰� ����

    void FillShopItemData(ShopItemForSend& shopItemData_) const override {

    }
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