#pragma once
#include <cstdint>

struct ShopItemForSend {
	uint32_t itemPrice = 0;
	uint16_t itemCode = 0;
	uint16_t itemCount = 1; // ������ ����
	uint16_t daysOrCount = 0; // [���: �Ⱓ, �Һ�: ���� ����] 
	uint16_t itemType;
	uint16_t currencyType; // ��������

	// ��� ������ �ʿ� ����
	uint16_t attackPower = 0;

	// �Һ� ������ �ʿ� ����

	// ��� ������ �ʿ� ����
};