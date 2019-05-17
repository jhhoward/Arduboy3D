#include "Defines.h"
#include "Map.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"

uint8_t level[MAP_WIDTH * MAP_HEIGHT / 2];

bool IsBlocked(uint8_t x, uint8_t y)
{
	return GetMapCellSafe(x, y) == 1;
}

bool IsSolid(uint8_t x, uint8_t y)
{
	return GetMapCellSafe(x, y) == 1;
}

uint8_t GetMapCell(uint8_t x, uint8_t y) 
{
	int index = y * MAP_WIDTH + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return cellData >> 4;
	}
	else
	{
		return cellData & 0xf;
	}
}

uint8_t GetMapCellSafe(uint8_t x, uint8_t y) 
{
	if(x >= MAP_WIDTH || y >= MAP_HEIGHT)
		return 1;
	
	int index = y * MAP_WIDTH + x;
	uint8_t cellData = level[index / 2];
	
	if(index & 1)
	{
		return cellData >> 4;
	}
	else
	{
		return cellData & 0xf;
	}
}

void SetMapCell(uint8_t x, uint8_t y, uint8_t cellType)
{
	if (x >= MAP_WIDTH || y >= MAP_HEIGHT)
	{
		return;
	}

	int index = (y * MAP_WIDTH + x) / 2;
	
	if(x & 1)
	{
		level[index] = (level[index] & 0xf) | (cellType << 4);
	}
	else
	{
		level[index] = (level[index] & 0xf0) | (cellType & 0xf);
	}
}

uint8_t CountNeighbours(uint8_t x, uint8_t y)
{
	uint8_t result = 0;
	
	if(GetMapCellSafe(x + 1, y) == 0)
		result++;
	if(GetMapCellSafe(x, y + 1) == 0)
		result++;
	if(GetMapCellSafe(x - 1, y) == 0)
		result++;
	if(GetMapCellSafe(x, y - 1) == 0)
		result++;
	if(GetMapCellSafe(x + 1, y + 1) == 0)
		result++;
	if(GetMapCellSafe(x - 1, y + 1) == 0)
		result++;
	if(GetMapCellSafe(x - 1, y - 1) == 0)
		result++;
	if(GetMapCellSafe(x + 1, y - 1) == 0)
		result++;
	
	return result;	
}

uint8_t CountImmediateNeighbours(uint8_t x, uint8_t y)
{
	uint8_t result = 0;

	if (GetMapCellSafe(x + 1, y) == 0)
		result++;
	if (GetMapCellSafe(x, y + 1) == 0)
		result++;
	if (GetMapCellSafe(x - 1, y) == 0)
		result++;
	if (GetMapCellSafe(x, y - 1) == 0)
		result++;

	return result;
}

struct NeighbourInfo
{
	union
	{
		uint8_t mask;

		struct  
		{
			bool hasNorth : 1;
			bool hasEast : 1;
			bool hasSouth : 1;
			bool hasWest : 1;

			bool canDemolishNorth : 1;
			bool canDemolishEast : 1;
			bool canDemolishSouth : 1;
			bool canDemolishWest : 1;
		};
	};

	uint8_t count;
};

void PullInCorner(uint8_t x, uint8_t y, uint8_t w, uint8_t h, int8_t dirX, int8_t dirY)
{
	uint8_t cornerSize = 1;

	for (uint8_t n = 0; n < w / 3 && n < h / 3; n++)
	{
		if (GetMapCell(x - dirX, y + dirY * n) == 0
		|| GetMapCell(x + dirX * n, y - dirY) == 0)
		{
			cornerSize--;
			break;
		}
		cornerSize++;
	}

	for (uint8_t n = 0; n < cornerSize; n++)
	{
		SetMapCell(x + dirX * n, y + dirY * cornerSize, 1);
		SetMapCell(x + dirX * cornerSize, y + dirY * n, 1);
	}
}

NeighbourInfo GetRoomNeighbourMask(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
	NeighbourInfo result;
	result.mask = 0;
	result.count = 0;

	result.canDemolishNorth = y > 1;
	result.canDemolishWest = x > 1;
	result.canDemolishEast = x + w + 1 < MAP_WIDTH - 1;
	result.canDemolishSouth = y + h + 1 < MAP_HEIGHT - 1;

	// Don't demolish walls if the neighbouring room has the same wall length
	if (GetMapCell(x - 1, y - 2) != 0 && GetMapCell(x + w, y - 2) != 0)
	{
		result.canDemolishNorth = false;
	}
	if (GetMapCell(x - 2, y - 1) != 0 && GetMapCell(x - 2, y + h) != 0)
	{
		result.canDemolishWest = false;
	}
	if (GetMapCell(x + w + 1, y - 1) != 0 && GetMapCell(x + w, y + h + 1) != 0)
	{
		result.canDemolishEast = false;
	}
	if (GetMapCell(x - 1, y + h + 1) != 0 && GetMapCell(x + w, y + h + 1) != 0)
	{
		result.canDemolishSouth = false;
	}

	// Don't demolish wall if this will leave an unattached wall
	if (GetMapCell(x - 1, y - 2) == 0 && GetMapCell(x - 2, y - 1) == 0)
	{
		result.canDemolishNorth = false;
		result.canDemolishWest = false;
	}
	if (GetMapCell(x + w, y - 2) == 0 && GetMapCell(x + w + 1, y - 1) == 0)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
	}
	if (GetMapCell(x + w, y + h + 1) == 0 && GetMapCell(x + w + 1, y + h) == 0)
	{
		result.canDemolishSouth = false;
		result.canDemolishEast = false;
	}
	if (GetMapCell(x - 1, y + h + 1) == 0 && GetMapCell(x - 2, y + h) == 0)
	{
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}

	bool hasNorthWall = GetMapCell(x, y - 1) != 0 && GetMapCell(x + w - 1, y - 1) != 0;
	bool hasEastWall = GetMapCell(x + w, y) != 0 && GetMapCell(x + w, y + h - 1) != 0;
	bool hasSouthWall = GetMapCell(x, y + h) != 0 && GetMapCell(x + w - 1, y + h) != 0;
	bool hasWestWall = GetMapCell(x - 1, y) != 0 && GetMapCell(x - 1, y + h - 1) != 0;

	if (!hasNorthWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
		result.canDemolishWest = false;
	}
	if (!hasEastWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishEast = false;
		result.canDemolishSouth = false;
	}
	if (!hasSouthWall)
	{
		result.canDemolishEast = false;
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}
	if (!hasWestWall)
	{
		result.canDemolishNorth = false;
		result.canDemolishSouth = false;
		result.canDemolishWest = false;
	}

	for (int i = x; i < x + w; i++)
	{
		if (GetMapCell(i, y - 1) == 0)
		{
			result.hasNorth = true;
			result.count++;
		}
		if (GetMapCell(i, y + h) == 0)
		{
			result.hasSouth = true;
			result.count++;
		}

		// Don't demolish wall if there is an intersecting wall attached
		if (y > 1 && GetMapCell(i, y - 2) != 0)
		{
			result.canDemolishNorth = false;
		}
		if (y + h + 1 < MAP_HEIGHT - 1 && GetMapCell(i, y + h + 1) != 0)
		{
			result.canDemolishSouth = false;
		}
	}
	for (int j = y; j < y + h; j++)
	{
		if (GetMapCell(x - 1, j) == 0)
		{
			result.hasWest = true;
			result.count++;
		}
		if (GetMapCell(x + w, j) == 0)
		{
			result.hasEast = true;
			result.count++;
		}
		
		// Don't demolish wall if there is an intersecting wall attached
		if (x > 1 && GetMapCell(x - 2, j) != 0)
		{
			result.canDemolishWest = false;
		}
		if (x + w + 1 < MAP_WIDTH - 1 && GetMapCell(x + w + 1, j) != 0)
		{
			result.canDemolishEast = false;
		}
	}

	return result;
}

void SplitMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t doorX, uint8_t doorY)
{
	constexpr int minRoomSize = 3;
	constexpr int maxRoomSize = 8;
	constexpr int maxFloorSpace = 80;
	constexpr int demolishWallChance = 20;

	if (doorX != 0 && doorY != 0)
	{
		SetMapCell(doorX, doorY, 0);
	}

	bool splitVertical = false;
	bool splitHorizontal = false;

	if (w > maxRoomSize || h > maxRoomSize)//w * h > maxFloorSpace)
	{
		if (w < h)
		{
			splitVertical = true;
		}
		else
		{
			splitHorizontal = true;
		}
	}

	if (splitVertical)
	{
		uint8_t splitSize;
		uint8_t splitAttempts = 255;
		do 
		{
			splitSize = (Random() % (h - 2 * minRoomSize)) + minRoomSize;
			splitAttempts--;
		} while (splitAttempts > 0 && (GetMapCell(x - 1, y + splitSize) == 0 || GetMapCell(x + w, y + splitSize) == 0
			|| GetMapCell(x - 1, y + splitSize - 1) == 0 || GetMapCell(x + w, y + splitSize - 1) == 0
			|| GetMapCell(x - 1, y + splitSize + 1) == 0 || GetMapCell(x + w, y + splitSize + 1) == 0));

		if (splitAttempts > 0)
		{
			uint8_t splitDoorX = x + (Random() % (w - 2)) + 1;
			uint8_t splitDoorY = y + splitSize;

			for (uint8_t i = x; i < x + w; i++)
			{
				SetMapCell(i, y + splitSize, 1);
			}

			SplitMap(x, y + splitSize + 1, w, h - splitSize - 1, splitDoorX, splitDoorY);
			SplitMap(x, y, w, splitSize, splitDoorX, splitDoorY);
			return;
		}
	}
	else if (splitHorizontal)
	{
		uint8_t splitSize;
		uint8_t splitAttempts = 255;
		do
		{
			splitSize = (Random() % (w - 2 * minRoomSize)) + minRoomSize;
			splitAttempts--;
		} while (splitAttempts > 0 && (GetMapCell(x + splitSize, y - 1) == 0 || GetMapCell(x + splitSize, y + h) == 0
			|| GetMapCell(x + splitSize - 1, y - 1) == 0 || GetMapCell(x + splitSize - 1, y + h) == 0
			|| GetMapCell(x + splitSize + 1, y - 1) == 0 || GetMapCell(x + splitSize + 1, y + h) == 0));
		
		if (splitAttempts > 0)
		{
			uint8_t splitDoorX = x + splitSize;
			uint8_t splitDoorY = y + (Random() % (h - 2)) + 1;

			for (uint8_t j = y; j < y + h; j++)
			{
				SetMapCell(x + splitSize, j, 1);
			}

			SplitMap(x + splitSize + 1, y, w - splitSize - 1, h, splitDoorX, splitDoorY);
			SplitMap(x, y, splitSize, h, splitDoorX, splitDoorY);
			return;
		}
	}

	{
		NeighbourInfo neighbours = GetRoomNeighbourMask(x, y, w, h);

		if (neighbours.canDemolishNorth && (Random() % 100) < demolishWallChance)
		{
			for (int i = 0; i < w; i++)
			{
				SetMapCell(x + i, y - 1, 0);
			}
		}
		else if (neighbours.canDemolishWest && (Random() % 100) < demolishWallChance)
		{
			for (int j = 0; j < h; j++)
			{
				SetMapCell(x - 1, y + j, 0);
			}
		}
		else if (neighbours.canDemolishSouth && (Random() % 100) < demolishWallChance)
		{
			for (int i = 0; i < w; i++)
			{
				SetMapCell(x + i, y + h, 0);
			}
		}
		else if (neighbours.canDemolishEast && (Random() % 100) < demolishWallChance)
		{
			for (int j = 0; j < h; j++)
			{
				SetMapCell(x + w, y + j, 0);
			}
		}

		// Add decorations
		{
			// Add four cornering columns
			if (w == h && w >= 7 && h >= 7)
			{
				SetMapCell(x + 1, y + 1, 1);
				SetMapCell(x + w - 2, y + 1, 1);
				SetMapCell(x + w - 2, y + h - 2, 1);
				SetMapCell(x + 1, y + h - 2, 1);
			}
		}
	}
}

void GenerateMap()
{
	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_WIDTH; x++)
		{
			bool isEdge = x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1;
			SetMapCell(x, y, isEdge ? 1 : 0);
		}
	}

	SplitMap(1, 1, MAP_WIDTH - 2, MAP_HEIGHT - 2, 0, 0);

	// Find any big open spaces
	{
		bool hasOpenSpaces = true;

		while (hasOpenSpaces)
		{
			hasOpenSpaces = false;

			uint8_t x = 0, y = 0, space = 0;

			for (uint8_t i = 1; i < MAP_WIDTH - 1; i++)
			{
				for (uint8_t j = 0; j < MAP_HEIGHT - 1; j++)
				{
					bool foundWall = false;

					for (uint8_t k = 0; k < MAP_HEIGHT && !foundWall; k++)
					{
						for (uint8_t u = 0; u < k && !foundWall; u++)
						{
							for (uint8_t v = 0; v < k && !foundWall; v++)
							{
								if (GetMapCellSafe(i + u, j + v) != 0)
								{
									foundWall = true;
								}
							}
						}

						if (!foundWall && k > space)
						{
							space = k;
							x = i;
							y = j;
						}
					}
				}
			}

			if (space > 6)
			{
				hasOpenSpaces = true;

				// Stick a donut in the middle
				for (uint8_t n = 2; n < space - 2; n++)
				{
					SetMapCell(x + n, y + 2, 1);
					SetMapCell(x + 2, y + n, 1);
					SetMapCell(x + n, y + space - 3, 1);
					SetMapCell(x + space - 3, y + n, 1);
				}
			}
		}
	}

	// Add torches
	{
		uint8_t attempts = 255;
		uint8_t toSpawn = 64;
		
		while(attempts > 0 && toSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;
			
			if(GetMapCellSafe(x, y) == 0)
			{
				uint8_t walls = 0;
				if(GetMapCellSafe(x + 1, y) == 1)
				{
					walls++;
				}
				if(GetMapCellSafe(x - 1, y) == 1)
				{
					walls++;
				}
				if(GetMapCellSafe(x, y + 1) == 1)
				{
					walls++;
				}
				if(GetMapCellSafe(x, y - 1) == 1)
				{
					walls++;
				}
				
				if(walls == 1)
				{
					SetMapCell(x, y, 3);
					toSpawn--;
					attempts = 255;
				}
			}
			
			attempts--;
		}
	}
	
	// Add monsters
	{
		uint8_t attempts = 255;
		uint8_t monstersToSpawn = 16;
		
		while(attempts > 0 && monstersToSpawn > 0)
		{
			uint8_t x = Random() % MAP_WIDTH;
			uint8_t y = Random() % MAP_HEIGHT;
			
			if(GetMapCellSafe(x, y) == 0 
			&& GetMapCellSafe(x + 1, y) == 0 
			&& GetMapCellSafe(x - 1, y) == 0 
			&& GetMapCellSafe(x, y + 1) == 0 
			&& GetMapCellSafe(x, y - 1) == 0)
			{
				SetMapCell(x, y, 2);
				monstersToSpawn--;
				attempts = 255;
			}
			
			attempts--;
		}
	}
	
}

void DrawMap()
{
	for(int y = 0; y < MAP_HEIGHT; y++)
	{
		for(int x = 0; x < MAP_WIDTH; x++)
		{
			PutPixel(x, y, GetMapCell(x, y) == 1 ? 1 : 0);

			if (x == camera.x / CELL_SIZE && y == camera.y / CELL_SIZE && (Renderer::globalAnimationFrame & 8) != 0)
			{
				PutPixel(x, y, 1);
			}
		}
	}
}
