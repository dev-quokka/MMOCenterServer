#pragma once
#include "ItemData.h"

class ItemDataManager { // Singleton class for managing item data
public:
    static ItemDataManager& GetInstance();

    bool LoadFromMySQL( // Mysql에서 데이터 로드 후 세팅
        std::unordered_map<uint16_t, EquipmentItemData>& em,
        std::unordered_map<uint16_t, ConsumableItemData>& cm,
        std::unordered_map<uint16_t, MaterialItemData>& mm
    );

    const EquipmentItemData* GetEquipment(uint16_t itemId) const;
    const ConsumableItemData* GetConsumable(uint16_t itemId) const; 
    const MaterialItemData* GetMaterial(uint16_t itemId) const; 

private:
    ItemDataManager() = default;
    ItemDataManager(const ItemDataManager&) = delete;
    ItemDataManager& operator=(const ItemDataManager&) = delete;

    std::unordered_map<uint16_t, EquipmentItemData> equipmentItemMap;
    std::unordered_map<uint16_t, ConsumableItemData> consumableItemMap;
    std::unordered_map<uint16_t, MaterialItemData> materialItemMap;

    bool loadCheck = false;
};