#include "Defines.h"
#include "Map.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Platform.h"

uint8_t Map::level[Map::width * Map::height / 2];

bool Map::IsBlocked(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) == CellType::BrickWall;
}

bool Map::IsSolid(uint8_t x, uint8_t y)
{
	return GetCellSafe(x, y) == CellType::BrickWall;
}

CellType Map::GetCell(uint8_t x, uint8_t y) 
{
	int index = y * Map::width + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return (CellType)(cellData >> 4);
	}
	else
	{
		return (CellType)(cellData & 0xf);
	}
}

CellType Map::GetCellSafe(uint8_t x, uint8_t y) 
{
	if(x >= Map::width || y >= Map::height)
		return CellType::BrickWall;
	
	int index = y * Map::width + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return (CellType)(cellData >> 4);
	}
	else
	{
		return (CellType)(cellData & 0xf);
	}
}

void Map::SetCell(uint8_t x, uint8_t y, CellType type)
{
	if (x >= Map::width || y >= Map::height)
	{
		return;
	}

	int index = (y * Map::width + x) / 2;
	uint8_t cellType = (uint8_t)type;
	
	if(x & 1)
	{
		level[index] = (level[index] & 0xf) | (cellType << 4);
	}
	else
	{
		level[index] = (level[index] & 0xf0) | (cellType & 0xf);
	}
}

void Map::DebugDraw()
{
	for(int y = 0; y < Map::height; y++)
	{
		for(int x = 0; x < Map::width; x++)
		{
			Platform::PutPixel(x, y, GetCell(x, y) == CellType::BrickWall ? 1 : 0);

			if (x == Renderer::camera.cellX && y == Renderer::camera.cellY && (Renderer::globalAnimationFrame & 8) != 0)
			{
				Platform::PutPixel(x, y, 1);
			}
		}
	}
}
