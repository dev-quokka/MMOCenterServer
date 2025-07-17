#pragma once
#include "ItemData.h"

class ItemDataManager { // Singleton class for managing item data
public:
    static ItemDataManager& GetInstance();

    // Mysql���� ������ �ε� �� ����
    bool LoadFromMySQL(std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash>& em);

    const ItemData* GetItemData(uint16_t itemId_, uint16_t itemType_) const;

private:
    ItemDataManager() = default;
    ItemDataManager(const ItemDataManager&) = delete;
    ItemDataManager& operator=(const ItemDataManager&) = delete;

    std::unordered_map<ItemDataKey, std::unique_ptr<ItemData>, ItemDataKeyHash> ItemMap;

    bool loadCheck = false;
};