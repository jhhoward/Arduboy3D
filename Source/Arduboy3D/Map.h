#pragma once

#include <stdint.h>

bool IsBlocked(uint8_t x, uint8_t y);
inline bool IsBlockedAtWorldPosition(int16_t x, int16_t y)
{
	return IsBlocked((uint8_t)(x >> 8), (uint8_t)(y >> 8));
}

uint8_t GetCellType(uint8_t x, uint8_t y);