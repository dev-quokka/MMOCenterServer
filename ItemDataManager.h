#pragma once
#include "ItemData.h"

class ItemDataManager { // Singleton class for managing item data
public:
    static ItemDataManager& GetInstance();

    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(std::unordered_map<uint16_t, std::unique_ptr<ItemData>>& em);

    const ItemData* GetItemData(uint16_t itemId) const;

private:
    ItemDataManager() = default;
    ItemDataManager(const ItemDataManager&) = delete;
    ItemDataManager& operator=(const ItemDataManager&) = delete;

    std::unordered_map<uint16_t, std::unique_ptr<ItemData>> ItemMap;

    bool loadCheck = false;
};