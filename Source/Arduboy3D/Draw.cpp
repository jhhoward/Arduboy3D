#include <stdint.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"
#include "Map.h"

#include "LUT.h"

#define WITH_TEXTURES 1

#if WITH_TEXTURES
#include "Textures.h"
#endif

Camera camera;

uint8_t wBuffer[DISPLAY_WIDTH];
uint8_t wallIdBuffer[DISPLAY_WIDTH];
int8_t horizonBuffer[DISPLAY_WIDTH];
uint8_t currentWallId = 0;

#if WITH_TEXTURES
inline void DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t col)
{
//	if(x1 < 0 || y1 < 0 || x2 >= DISPLAY_WIDTH || y2 >= DISPLAY_HEIGHT)
//		return;

	if(x1 > x2)
		return;
	
	if (y1 < 0)
	{
		if(y2 < 0)
			return;
		
		if(y2 != y1)
			x1 += (0 - y1) * (x2 - x1) / (y2 - y1);
		y1 = 0;
	}
	if (y2 > DISPLAY_HEIGHT - 1)
	{
		if(y1 > DISPLAY_HEIGHT - 1)
			return;
		
		if(y2 != y1)
			x2 += (((DISPLAY_HEIGHT - 1) - y2) * (x1 - x2)) / (y1 - y2);
		y2 = DISPLAY_HEIGHT - 1;
	}
	
	if (x1 < 0)
	{
		if(x2 != x1)
		{
			y1 += ((0 - x1) * (y2 - y1)) / (x2 - x1);
		}
		x1 = 0;
	}

	int16_t dx = x2 - x1;
	int16_t yerror = dx / 2;
	int16_t y = y1;
	int16_t dy;
	int8_t ystep;

	if (y1 < y2)
	{
		dy = y2 - y1;
		ystep = 1;
	}
	else
	{
		dy = y1 - y2;
		ystep = -1;
	}

	for (int x = x1; x <= x2 && x < DISPLAY_WIDTH; x++)
	{
		//int w = y > HORIZON ? y - HORIZON : HORIZON - y;
		bool drawSlice = wallIdBuffer[x] == currentWallId;// && wBuffer[x] >= w;
		int8_t horizon = horizonBuffer[x] - HORIZON;		

		if (drawSlice)
		{
			PutPixel(x, horizon + y, col);
		}

		yerror -= dy;

		while (yerror < 0)
		{
			y += ystep;
			
			//if(y < 0 || y >= DISPLAY_HEIGHT)
			//	return;
			
			yerror += dx;

			if (drawSlice && yerror < 0)
			{
				PutPixel(x, horizon + y, col);
			}

			if (x == x2 && y == y2)
				break;
		}
	}
}
#endif

#if WITH_TEXTURES
inline void DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
inline void DrawWallSegment(int16_t x1, int16_t w1, int16_t x2, int16_t w2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	if (x1 < 0)
	{
#if WITH_TEXTURES
		u1clip += ((int32_t)(0 - x1) * (int32_t)(u2clip - u1clip)) / (x2 - x1);
#endif
		w1 += ((int32_t)(0 - x1) * (int32_t)(w2 - w1)) / (x2 - x1);
		x1 = 0;
		edgeLeft = false;
	}

	int16_t dx = x2 - x1;
	int16_t werror = dx / 2;
	int16_t w = w1;
	int16_t dw;
	int8_t wstep;

	currentWallId++;

	if (w1 < w2)
	{
		dw = w2 - w1;
		wstep = 1;
	}
	else
	{
		dw = w1 - w2;
		wstep = -1;
	}

	constexpr uint8_t wallColour = COLOUR_WHITE;
	constexpr uint8_t edgeColour = COLOUR_BLACK;

	for (int x = x1; x < DISPLAY_WIDTH; x++)
	{
		bool drawSlice = x >= 0 && wBuffer[x] < w;
		bool shadeSlice = shadeEdge && (x & 1) == 0;		
		
		int8_t horizon = horizonBuffer[x];

		if (drawSlice)
		{
			uint8_t sliceMask = 0xff;
			uint8_t sliceColour = wallColour;

			if ((edgeLeft && x == x1) || (edgeRight && x == x2))
			{
				sliceMask = 0x00;
				sliceColour = edgeColour;
			}
			else if (shadeSlice)
			{
				sliceMask = 0x55;
			}

			int8_t extent = w > 64 ? 64 : w;
			DrawVLine(x, horizon - extent, horizon + extent, sliceMask);
			PutPixel(x, horizon + extent, edgeColour);
			PutPixel(x, horizon - extent, edgeColour);

			wallIdBuffer[x] = currentWallId;
			if (w > 255)
				wBuffer[x] = 255;
			else
				wBuffer[x] = w;
		}

		if (x == x2)
			break;

		werror -= dw;

		while (werror < 0)
		{
			w += wstep;
			werror += dx;

			if (drawSlice && werror < 0 && w <= DISPLAY_HEIGHT / 2)
			{
				PutPixel(x, horizon + w - 1, edgeColour);
				PutPixel(x, horizon - w, edgeColour);
			}
		}
	}

#if WITH_TEXTURES
	if (w1 < MIN_TEXTURE_DISTANCE || w2 < MIN_TEXTURE_DISTANCE || !texture)
		return;
	if(u1clip == u2clip)
		return;

	const uint8_t* texPtr = texture;
	uint8_t numLines = pgm_read_byte(texPtr++);
	while (numLines)
	{
		numLines--;
		
		uint8_t u1 = pgm_read_byte(texPtr++);
		uint8_t v1 = pgm_read_byte(texPtr++);
		uint8_t u2 = pgm_read_byte(texPtr++);
		uint8_t v2 = pgm_read_byte(texPtr++);

		//if(u1clip != 0 || u2clip != 128)
		//	continue;
		
		if (u2 < u1clip || u1 > u2clip)
			continue;

		if (u1 < u1clip)
		{
			if(u2 != u1)
				v1 += (u1clip - u1) * (v2 - v1) / (u2 - u1);
			u1 = u1clip;
		}
		if (u2 > u2clip)
		{
			if (u2 != u1)
				v2 += (u2clip - u2) * (v1 - v2) / (u1 - u2);
			u2 = u2clip;
		}
		
		u1 = (128 * (u1 - u1clip)) / (u2clip - u1clip);
		u2 = (128 * (u2 - u1clip)) / (u2clip - u1clip);

		int16_t outU1 = (((int32_t)u1 * dx) >> 7) + x1;
		int16_t outU2 = (((int32_t)u2 * dx) >> 7) + x1;

		int16_t interpw1 = ((u1 * (w2 - w1)) >> 7) + w1;
		int16_t interpw2 = ((u2 * (w2 - w1)) >> 7) + w1;

		int16_t outV1 = (interpw1 * v1) >> 6;
		int16_t outV2 = (interpw2 * v2) >> 6;

		//uint8_t horizon = horizonBuffer[x]
		//DrawLine(ScreenSurface, outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour, edgeColour, edgeColour);
		DrawWallLine(outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour);
		//DrawWallLine(outU1, -interpw1 + outV1, outU2, -interpw2 + outV2, edgeColour);
	}
#endif
}

inline bool isFrustrumClipped(int16_t x, int16_t y)
{
	if ((camera.clipCos * (x - camera.cellX) - camera.clipSin * (y - camera.cellY)) < -512)
		return true;
	if ((camera.clipSin * (x - camera.cellX) + camera.clipCos * (y - camera.cellY)) < -512)
		return true;

	return false;
}

inline void TransformToViewSpace(int16_t x, int16_t y, int16_t* outX, int16_t* outY)
{
	int32_t relX = x - camera.x;
	int32_t relY = y - camera.y;
	*outY = (int16_t)((camera.rotCos * relX) >> 8) - (int16_t)((camera.rotSin * relY) >> 8);
	*outX = (int16_t)((camera.rotSin * relX) >> 8) + (int16_t)((camera.rotCos * relY) >> 8);
}

inline void TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t* outX, int16_t* outW)
{
	// apply perspective projection
	*outX = (int16_t)((int32_t)viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
	*outW = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ);

	// transform into screen space
	*outX = (int16_t)((DISPLAY_WIDTH / 2) + *outX);
}

#if WITH_TEXTURES
void DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
void DrawWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;
#if WITH_TEXTURES
	uint8_t u1 = 0, u2 = 128;
#endif

	TransformToViewSpace(x1, y1, &viewX1, &viewZ1);
	TransformToViewSpace(x2, y2, &viewX2, &viewZ2);

	// Frustum cull
	if (viewX2 < 0 && -2 * viewZ2 > viewX2)
		return;
	if (viewX1 > 0 && 2 * viewZ1 < viewX1)
		return;

	// clip to the front pane
	if ((viewZ1 < CLIP_PLANE) && (viewZ2 < CLIP_PLANE))
		return;

	if (viewZ1 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u1 += (CLIP_PLANE - viewZ1) * (u2 - u1) / (viewZ2 - viewZ1);
#endif
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
		edgeLeft = false;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
#if WITH_TEXTURES
		u2 += (CLIP_PLANE - viewZ2) * (u1 - u2) / (viewZ1 - viewZ2);
#endif
		viewX2 += (CLIP_PLANE - viewZ2) * (viewX1 - viewX2) / (viewZ1 - viewZ2);
		viewZ2 = CLIP_PLANE;
		edgeRight = false;
	}

	// apply perspective projection
	int16_t vx1 = (int16_t)((int32_t)viewX1 * NEAR_PLANE * CAMERA_SCALE / viewZ1);
	int16_t vx2 = (int16_t)((int32_t)viewX2 * NEAR_PLANE * CAMERA_SCALE / viewZ2);

	// transform the end points into screen space
	int16_t sx1 = (int16_t)((DISPLAY_WIDTH / 2) + vx1);
	int16_t sx2 = (int16_t)((DISPLAY_WIDTH / 2) + vx2) - 1;

	if (sx1 >= sx2 || sx2 < 0 || sx1 >= DISPLAY_WIDTH)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

#if WITH_TEXTURES
	DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, edgeLeft, edgeRight, shadeEdge);
#else
	DrawWallSegment(sx1, w1, sx2, w2, edgeLeft, edgeRight, shadeEdge);
#endif
}

void DrawCell(uint8_t x, uint8_t y)
{
	if (!IsBlocked(x, y))
	{
		return;
	}

	if (isFrustrumClipped(x, y))
	{
		return;
	}

	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = IsBlocked(x - 1, y);
	bool blockedRight = IsBlocked(x + 1, y);
	bool blockedUp = IsBlocked(x, y - 1);
	bool blockedDown = IsBlocked(x, y + 1);

#if WITH_TEXTURES
	uint8_t cellType = GetCellType(x, y);
	const uint8_t* texture = (const uint8_t*) pgm_read_ptr(&textures[cellType - 1]);
//	texture = nullptr;
#endif

	if (!blockedLeft && camera.x < x1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2, true);
#else
		DrawWall(x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2, true);
#endif
	}

	if (!blockedDown && camera.y > y2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2, false);
#else
		DrawWall(x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2, false);
#endif
	}

	if (!blockedRight && camera.x > x2)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1, true);
#else
		DrawWall(x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1, true);
#endif
	}

	if (!blockedUp && camera.y < y1)
	{
#if WITH_TEXTURES
		DrawWall(texture, x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1, false);
#else
		DrawWall(x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1, false);
#endif
	}
}

void DrawCells()
{
	for (uint8_t y = 0; y < MAP_SIZE; y++)
	{
		for (uint8_t x = 0; x < MAP_SIZE; x++)
		{
			DrawCell(x, y);
		}
	}
}

const uint8_t testSprite[] PROGMEM =
{
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
	0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
};

void DrawScaled2x(const uint8_t* data, int x, int y, uint8_t halfSize)
{
	uint8_t size = 2 * halfSize;

	if (size > MAX_SPRITE_SIZE * 2)
	{
		return;
	}

	const uint8_t* lut = scaleLUT + ((halfSize / 2) * (halfSize / 2));

	uint8_t u0 = 0;
	uint8_t u1 = 0;
	uint8_t u2;
	uint8_t up, px, down;
	int outX = x;

	for (int i = 0; i < size && outX < DISPLAY_WIDTH; i++)
	{
		uint8_t v0 = 0;
		uint8_t v1 = 0;
		uint8_t v2;

		u2 = pgm_read_byte(&lut[(i + 1) / 2]);

		up = 0;
		px = pgm_read_byte(&data[0]);

		if (outX >= 0 && wBuffer[outX] < halfSize)
		{
			int outY = y;

			for (int j = 0; j < size && outY < DISPLAY_HEIGHT; j++)
			{
				v2 = pgm_read_byte(&lut[(j + 1) / 2]);
				down = pgm_read_byte(&data[v2 * BASE_SPRITE_SIZE + u1]);

				if (outY >= 0 && outY < DISPLAY_HEIGHT)
				{
					if (px)
					{
						if (i == 0 || j == 0 || i == size - 1 || j == size - 1)
						{
							PutPixel(outX, outY, COLOUR_BLACK);
						}
						else if (px == 2)
						{
							PutPixel(outX, outY, COLOUR_BLACK);
						}
						else
						{
							PutPixel(outX, outY, COLOUR_WHITE);
						}
					}
					else
					{
						uint8_t left = pgm_read_byte(&data[v1 * BASE_SPRITE_SIZE + u0]);
						uint8_t right = pgm_read_byte(&data[v1 * BASE_SPRITE_SIZE + u2]);

						if (up | down | left | right)
						{
							PutPixel(outX, outY, COLOUR_BLACK);
						}
					}
				}

				v0 = v1;
				v1 = v2;

				up = px;
				px = down;
				outY++;
			}
		}

		u0 = u1;
		u1 = u2;
		outX++;
	}
}

void DrawScaled(const uint8_t* data, int x, int y, uint8_t halfSize)
{
	if (halfSize < 2)
	{
		return;
	}

	uint8_t size = halfSize * 2;

	if (size > MAX_SPRITE_SIZE)
	{
		DrawScaled2x(data, x, y, halfSize);
		return;
	}

	const uint8_t* lut = scaleLUT + ((halfSize) * (halfSize));

	uint8_t u0 = 0;
	uint8_t u1 = 0;
	uint8_t u2;
	uint8_t up, px, down;
	int outX = x;

	for (int i = 0; i < size && outX < DISPLAY_WIDTH; i++)
	{
		uint8_t v0 = 0;
		uint8_t v1 = 0;
		uint8_t v2;

		u2 = pgm_read_byte(&lut[i + 1]);

		up = 0;
		px = pgm_read_byte(&data[0]);

		if (outX >= 0 && wBuffer[outX] < halfSize)
		{
			int outY = y;

			for (int j = 0; j < size && outY < DISPLAY_HEIGHT; j++)
			{
				v2 = pgm_read_byte(&lut[j + 1]);
				down = pgm_read_byte(&data[v2 * BASE_SPRITE_SIZE + u1]);

				if (px)
				{
					if (i == 0 || j == 0 || i == size - 1 || j == size - 1)
					{
						PutPixel(outX, outY, COLOUR_BLACK);
					}
					else if (px == 2)
					{
						PutPixel(outX, outY, COLOUR_BLACK);
					}
					else
					{
						PutPixel(outX, outY, COLOUR_WHITE);
					}
				}
				else
				{
					uint8_t left = pgm_read_byte(&data[v1 * BASE_SPRITE_SIZE + u0]);
					uint8_t right = pgm_read_byte(&data[v1 * BASE_SPRITE_SIZE + u2]);

					if (up | down | left | right)
					{
						PutPixel(outX, outY, COLOUR_BLACK);
					}
				}

				v0 = v1;
				v1 = v2;

				up = px;
				px = down;
				outY++;
			}
		}

		u0 = u1;
		u1 = u2;
		outX++;
	}
}

inline int8_t GetHorizon(int16_t x)
{
	if (x < 0)
		x = 0;
	if (x >= DISPLAY_WIDTH)
		x = DISPLAY_WIDTH - 1;
	return horizonBuffer[x];
}

void DrawObject(int16_t x, int16_t y)
{
	int16_t relX, relZ;
	int16_t screenX, screenW;

	TransformToViewSpace(x, y, &relX, &relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return;

	if (relX < 0 && -2 * relZ > relX)
		return;
	if (relX > 0 && 2 * relZ < relX)
		return;

	TransformToScreenSpace(relX, relZ, &screenX, &screenW);

	DrawScaled(testSprite, screenX - screenW, GetHorizon(screenX) - screenW, (uint8_t) screenW);
}

void DrawParticleSystem(ParticleSystem* system, int16_t x, int16_t y)
{
	int16_t relX, relZ;
	int16_t screenX, screenW;

	TransformToViewSpace(x, y, &relX, &relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return;

	if (relX < 0 && -2 * relZ > relX)
		return;
	if (relX > 0 && 2 * relZ < relX)
		return;

	TransformToScreenSpace(relX, relZ, &screenX, &screenW);

	system->Draw(screenX, screenW);
}

ParticleSystem testParticles;

void Render()
{
	DrawBackground();
	
	currentWallId = 0;
	for (uint8_t n = 0; n < DISPLAY_WIDTH; n++)
	{
		wallIdBuffer[n] = 0;
		wBuffer[n] = 0;
		horizonBuffer[n] = HORIZON + (((DISPLAY_WIDTH / 2 - n) * camera.tilt) >> 8);
	}

	camera.cellX = camera.x / CELL_SIZE;
	camera.cellY = camera.y / CELL_SIZE;

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);

#if !WITH_TEXTURES
	uint8_t doorX = 3;
	uint8_t doorY = 4;
	DrawWall(doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE, 
			 doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE / 2, true, true, false);
	DrawWall(doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE / 2,
			 doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE, true, true, false);
	DrawWall(doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE / 2, 
			 doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE, true, true, false);
	DrawWall(doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE,
			 doorX * CELL_SIZE + CELL_SIZE / 2, doorY * CELL_SIZE + CELL_SIZE / 2, true, true, false);
#endif

	DrawCells();

	DrawObject(3 * CELL_SIZE + CELL_SIZE / 2, 1 * CELL_SIZE + CELL_SIZE / 2);
	DrawObject(1 * CELL_SIZE + CELL_SIZE / 2, 5 * CELL_SIZE + CELL_SIZE / 2);

	static int counter = 0;
	counter++;
	if (counter == 20)
	{
		testParticles.Explode(4);
		counter = 0;
	}
	testParticles.Step();
	DrawParticleSystem(&testParticles, 1 * CELL_SIZE + CELL_SIZE / 2, 1 * CELL_SIZE + CELL_SIZE / 2);
}
