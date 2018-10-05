#include "Defines.h"
#include "Map.h"

const uint8_t map[] PROGMEM =
{
	1, 1, 1, 2, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 1, 0, 1,
	1, 0, 0, 1, 3, 1, 0, 1,
	1, 1, 0, 0, 0, 1, 0, 1,
	1, 0, 0, 1, 0, 1, 0, 1,
	1, 1, 0, 1, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1,
};

//const uint8_t map[] PROGMEM =
//{
//	1, 1, 1, 1, 1, 1, 1, 1,
//	1, 0, 0, 0, 0, 0, 0, 1,
//	1, 0, 0, 0, 0, 0, 0, 1,
//	1, 0, 0, 1, 1, 0, 0, 1,
//	1, 0, 0, 1, 1, 0, 0, 1,
//	1, 0, 0, 0, 0, 0, 0, 1,
//	1, 0, 0, 0, 0, 0, 0, 1,
//	1, 1, 1, 1, 1, 1, 1, 1,
//};

bool IsBlocked(uint8_t x, uint8_t y)
{
	if (x >= MAP_SIZE || y >= MAP_SIZE)
	{
		return true;
	}
	return pgm_read_byte(&map[y * MAP_SIZE + x]) != 0;
}

uint8_t GetCellType(uint8_t x, uint8_t y)
{
	if (x >= MAP_SIZE || y >= MAP_SIZE)
	{
		return 1;
	}
	return pgm_read_byte(&map[y * MAP_SIZE + x]);
}
