#pragma once
#include "ShopItemData.h"
#include "ItemDataManager.h"

class ShopDataManager { // Singleton class for managing shop data
public:
	static ShopDataManager& GetInstance();

	bool LoadFromMySQL( // MySQL에서 상점 데이터를 로드한 뒤, 각 아이템 코드에 해당하는 ItemData 정보를 매칭하여 저장
		std::unordered_map<ShopEquipmentKey, ShopEquipmentItem, ShopEquipmentKeyHash>& em,
		std::unordered_map<uint16_t, ShopConsumableItem>& cm,
		std::unordered_map<uint16_t, ShopMaterialItem>& mm
	);

	// 유저가 상점에서 특정 아이템을 선택했을 때 해당 아이템 정보를 반환
	const ShopEquipmentItem* GetEquipment(uint16_t itemId, uint16_t days) const;
	const ShopConsumableItem* GetConsumable(uint16_t itemId) const;
	const ShopMaterialItem* GetMaterial(uint16_t itemId) const;

	// 유저 접속 시 전체 상점 정보를 전달하기 위한 벡터 반환
	const std::vector<ShopEquipmentItem>& GetEquipmentVector() const;
	const std::vector<ShopConsumableItem>& GetConsumableVector() const;
	const std::vector<ShopMaterialItem>& GetMaterialVector() const;

private:
	ShopDataManager() = default;
	ShopDataManager(const ShopDataManager&) = delete;
	ShopDataManager& operator=(const ShopDataManager&) = delete;

	std::unordered_map<ShopEquipmentKey, ShopEquipmentItem, ShopEquipmentKeyHash> shopEquipmentItemMap; // { 아이템 코드, 사용 기한 }
	std::unordered_map<uint16_t, ShopConsumableItem> shopConsumableItemMap;
	std::unordered_map<uint16_t, ShopMaterialItem> shopMaterialItemMap;

	std::vector<ShopEquipmentItem> shopEquipmentItemVector;
	std::vector<ShopConsumableItem> shopConsumableItemVector;
	std::vector<ShopMaterialItem> shopMaterialItemVector;

	bool loadCheck = false;
};