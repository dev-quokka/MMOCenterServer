#pragma once
#include "ShopItemData.h"
#include "ItemDataManager.h"

class ShopDataManager { // Singleton class for managing shop data
public:
	static ShopDataManager& GetInstance();

	bool LoadFromMySQL( // MySQL���� ���� �����͸� �ε��� ��, �� ������ �ڵ忡 �ش��ϴ� ItemData ������ ��Ī�Ͽ� ����
		std::unordered_map<ShopEquipmentKey, ShopEquipmentItem, ShopEquipmentKeyHash>& em,
		std::unordered_map<uint16_t, ShopConsumableItem>& cm,
		std::unordered_map<uint16_t, ShopMaterialItem>& mm
	);

	// ������ �������� Ư�� �������� �������� �� �ش� ������ ������ ��ȯ
	const ShopEquipmentItem* GetEquipment(uint16_t itemId, uint16_t days) const;
	const ShopConsumableItem* GetConsumable(uint16_t itemId) const;
	const ShopMaterialItem* GetMaterial(uint16_t itemId) const;

	// ���� ���� �� ��ü ���� ������ �����ϱ� ���� ���� ��ȯ
	const std::vector<ShopEquipmentItem>& GetEquipmentVector() const;
	const std::vector<ShopConsumableItem>& GetConsumableVector() const;
	const std::vector<ShopMaterialItem>& GetMaterialVector() const;

private:
	ShopDataManager() = default;
	ShopDataManager(const ShopDataManager&) = delete;
	ShopDataManager& operator=(const ShopDataManager&) = delete;

	std::unordered_map<ShopEquipmentKey, ShopEquipmentItem, ShopEquipmentKeyHash> shopEquipmentItemMap; // { ������ �ڵ�, ��� ���� }
	std::unordered_map<uint16_t, ShopConsumableItem> shopConsumableItemMap;
	std::unordered_map<uint16_t, ShopMaterialItem> shopMaterialItemMap;

	std::vector<ShopEquipmentItem> shopEquipmentItemVector;
	std::vector<ShopConsumableItem> shopConsumableItemVector;
	std::vector<ShopMaterialItem> shopMaterialItemVector;

	bool loadCheck = false;
};