#pragma once

#include <stdint.h>

class MapGenerator
{
public:
	static void Generate();

private:

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

	static uint8_t CountNeighbours(uint8_t x, uint8_t y);
	static uint8_t CountImmediateNeighbours(uint8_t x, uint8_t y);
	static NeighbourInfo GetRoomNeighbourMask(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
	static void SplitMap(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t doorX, uint8_t doorY);

};
