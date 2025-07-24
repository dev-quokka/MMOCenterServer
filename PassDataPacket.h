#pragma once
#include <cstdint>

constexpr uint16_t MAX_PASS_ID_LEN = 32;

struct PassDataForSend {
    char itemName[MAX_ITEM_ID_LEN + 1];
    char passId[MAX_PASS_ID_LEN + 1];
    uint16_t itemCode = 0;
    uint16_t passLevel = 0;
    uint16_t itemCount = 1; // 아이템 개수
    uint16_t daysOrCount = 0;
    uint16_t itemType;
    uint16_t passCurrencyType;
};