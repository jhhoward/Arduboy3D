#include <stdint.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"

#include "LUT.h"
#include "Textures.h"

Camera camera;

const uint8_t map[] PROGMEM =
{
	1, 1, 1, 1, 2, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 3, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 1,
	2, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 3, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 2,
	1, 1, 1, 1, 1, 2, 1, 1,
};

uint8_t wBuffer[DISPLAY_WIDTH];
uint8_t wallIdBuffer[DISPLAY_WIDTH];
uint8_t currentWallId = 0;

inline void DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t col)
{
	if (y1 < 0)
	{
		if(y2 != y1)
			x1 += (0 - y1) * (x2 - x1) / (y2 - y1);
		y1 = 0;
	}
	if (x1 < 0)
	{
		if(x2 != x1)
			y1 += (0 - x1) * (y2 - y1) / (x2 - x1);
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
		int w = y > HORIZON ? y - HORIZON : HORIZON - y;
		bool drawSlice = wallIdBuffer[x] == currentWallId && wBuffer[x] >= w;

		if (drawSlice)
		{
			PutPixel(x, y, col);
		}

		yerror -= dy;

		while (yerror < 0)
		{
			y += ystep;
			yerror += dx;

			if (drawSlice && yerror < 0)
			{
				PutPixel(x, y, col);
			}

			if (x == x2 && y == y2)
				break;
		}
	}
}

inline void DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight)
{
	int16_t origX1 = x1;
	int16_t origW1 = w1;
	int16_t origDx = x2 - x1;

	if (x1 < 0)
	{
		u1clip += (0 - x1) * (u2clip - u1clip) / (x2 - x1);
		w1 += (0 - x1) * (w2 - w1) / (x2 - x1);
		x1 = 0;
	}

	int16_t dx = x2 - x1;
	int16_t werror = dx / 2;
	int16_t w = w1;
	int16_t dw;
	int8_t wstep;

	//if (viewZ1 < CLIP_PLANE)
	//{
	//	viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
	//	viewZ1 = CLIP_PLANE;
	//}

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

	uint8_t wallColour = COLOUR_WHITE;
	uint8_t edgeColour = COLOUR_BLACK;

	for (int x = x1; x < DISPLAY_WIDTH; x++)
	{
		bool drawSlice = x >= 0 && wBuffer[x] < w;

		if (drawSlice)
		{
			uint8_t sliceColour = ((edgeLeft && x == x1) || (edgeRight && x == x2)) ? edgeColour : wallColour;

			PutPixel(x, HORIZON, sliceColour);
			for (int j = 1; j < w && j <= DISPLAY_HEIGHT / 2; j++)
			{
				PutPixel(x, HORIZON + j, sliceColour);
				PutPixel(x, HORIZON - j, sliceColour);
			}

			if (w <= DISPLAY_HEIGHT / 2)
			{
				PutPixel(x, HORIZON + w, edgeColour);
				PutPixel(x, HORIZON - w, edgeColour);
			}

			wallIdBuffer[x] = currentWallId;
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
				PutPixel(x, HORIZON + w, edgeColour);
				PutPixel(x, HORIZON - w, edgeColour);
			}
		}
	}

	if (w1 < MIN_TEXTURE_DISTANCE || w2 < MIN_TEXTURE_DISTANCE || 1)
		return;

	const uint8_t* texPtr = texture;
	uint8_t numLines = pgm_read_byte(texPtr++);
	while (numLines)
	{
		uint8_t u1 = pgm_read_byte(texPtr++);
		uint8_t v1 = pgm_read_byte(texPtr++);
		uint8_t u2 = pgm_read_byte(texPtr++);
		uint8_t v2 = pgm_read_byte(texPtr++);

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

		int outU1 = ((u1 * origDx) >> 7) + origX1;
		int outU2 = ((u2 * origDx) >> 7) + origX1;
		int interpw1 = ((u1 * (w2 - origW1)) >> 7) + origW1;
		int interpw2 = ((u2 * (w2 - origW1)) >> 7) + origW1;

		int outV1 = (interpw1 * v1 * 2) >> 7;
		int outV2 = (interpw2 * v2 * 2) >> 7;

		//DrawLine(ScreenSurface, outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour, edgeColour, edgeColour);
		DrawWallLine(outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, edgeColour);

		numLines--;
	}
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

void DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight)
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;
	uint8_t u1 = 0, u2 = 128;

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
		u1 += (CLIP_PLANE - viewZ1) * (u2 - u1) / (viewZ2 - viewZ1);
		viewX1 += (CLIP_PLANE - viewZ1) * (viewX2 - viewX1) / (viewZ2 - viewZ1);
		viewZ1 = CLIP_PLANE;
		edgeLeft = false;
	}
	else if (viewZ2 < CLIP_PLANE)
	{
		u2 += (CLIP_PLANE - viewZ2) * (u2 - u1) / (viewZ1 - viewZ2);
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

	DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, edgeLeft, edgeRight);
}

bool IsBlocked(uint8_t x, uint8_t y)
{
	if (x >= MAP_SIZE || y >= MAP_SIZE)
	{
		return true;
	}
	return pgm_read_byte(&map[y * MAP_SIZE + x]) != 0;
}

void DrawCell(uint8_t x, uint8_t y)
{
	if (isFrustrumClipped(x, y))
	{
		return;
	}

	uint8_t cellType = pgm_read_byte(&map[y * MAP_SIZE + x]);

	if (!cellType)
		return;

	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = IsBlocked(x - 1, y);
	bool blockedRight = IsBlocked(x + 1, y);
	bool blockedUp = IsBlocked(x, y - 1);
	bool blockedDown = IsBlocked(x, y + 1);

	const uint8_t* texture = (const uint8_t*) pgm_read_ptr(&textures[cellType - 1]);

	if (!blockedLeft && camera.x < x1)
	{
		DrawWall(texture, x1, y1, x1, y2, !blockedUp && camera.y > y1, !blockedDown && camera.y < y2);
	}

	if (!blockedDown && camera.y > y2)
	{
		DrawWall(texture, x1, y2, x2, y2, !blockedLeft && camera.x > x1, !blockedRight && camera.x < x2);
	}

	if (!blockedRight && camera.x > x2)
	{
		DrawWall(texture, x2, y2, x2, y1, !blockedDown && camera.y < y2, !blockedUp && camera.y > y1);
	}

	if (!blockedUp && camera.y < y1)
	{
		DrawWall(texture, x2, y1, x1, y1, !blockedRight && camera.x < x2, !blockedLeft && camera.x > x1);
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

		if (outX >= 0 && wBuffer[outX] < size)
		{
			int outY = y;

			for (int j = 0; j < size && outY < DISPLAY_HEIGHT; j++)
			{
				v2 = pgm_read_byte(&lut[(j + 1) / 2]);
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

		if (outX >= 0 && wBuffer[outX] < size)
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

	DrawScaled(testSprite, screenX - screenW, HORIZON - screenW, (uint8_t) screenW);
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
	for (int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			uint8_t col = y < DISPLAY_HEIGHT / 2 ? (x | y) & 1 ? COLOUR_BLACK : COLOUR_WHITE : (x ^ y) & 1 ? COLOUR_BLACK : COLOUR_WHITE; //192;
																								 //col = 192;
			PutPixel(x, y, col);
		}
	}
	
	currentWallId = 0;
	for (uint8_t n = 0; n < DISPLAY_WIDTH; n++)
	{
		wallIdBuffer[n] = 0;
		wBuffer[n] = 0;
	}

	camera.cellX = camera.x / CELL_SIZE;
	camera.cellY = camera.y / CELL_SIZE;

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);


	DrawCells();

	DrawObject(3 * CELL_SIZE, 3 * CELL_SIZE);
	DrawObject(5 * CELL_SIZE, 4 * CELL_SIZE);

	static int counter = 0;
	counter++;
	if (counter == 20)
	{
		testParticles.Explode(4);
		counter = 0;
	}
	testParticles.Step();
	DrawParticleSystem(&testParticles, 5 * CELL_SIZE, 5 * CELL_SIZE);

}
