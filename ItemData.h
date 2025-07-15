#pragma once
#include <unordered_map>
#include <string>
#include <iostream>

struct EquipmentItemData {
    std::string itemName;
    uint16_t itemCode;
    uint16_t attackPower;

    // ���� �ʿ��� ������(���� ���� �� ��� ����) �߰� ����
};

struct ConsumableItemData {
    std::string itemName;
    uint16_t itemCode;

    // ���� �ʿ��� ������ �߰� ����
};

struct MaterialItemData {
    std::string itemName;
    uint16_t itemCode;

    // ���� �ʿ��� ������ �߰� ����
};