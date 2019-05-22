#include <stdint.h>
#include "Draw.h"
#include "Defines.h"
#include "Game.h"
#include "Particle.h"
#include "FixedMath.h"
#include "Map.h"
#include "Projectile.h"
#include "Platform.h"

#include "LUT.h"
#include "Generated/SpriteData.inc.h"

#if WITH_VECTOR_TEXTURES
#include "Textures.h"
#endif

#if WITH_SPRITE_OUTLINES
#define DrawScaledInner DrawScaledOutline
#else
#define DrawScaledInner DrawScaledNoOutline
#endif

Camera Renderer::camera;
uint8_t Renderer::wBuffer[DISPLAY_WIDTH];
int8_t Renderer::horizonBuffer[DISPLAY_WIDTH];
uint8_t Renderer::globalAnimationFrame = 0;
uint8_t Renderer::numBufferSlicesFilled = 0;
QueuedDrawable Renderer::queuedDrawables[MAX_QUEUED_DRAWABLES];
uint8_t Renderer::numQueuedDrawables = 0;

const uint8_t scaleDrawWriteMasks[] PROGMEM =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7)
};

const uint16_t scaleDrawReadMasks[] PROGMEM =
{
	(1),
	(1 << 1),
	(1 << 2),
	(1 << 3),
	(1 << 4),
	(1 << 5),
	(1 << 6),
	(1 << 7),
	(1 << 8),
	(1 << 9),
	(1 << 10),
	(1 << 11),
	(1 << 12),
	(1 << 13),
	(1 << 14),
	(1 << 15)
};

#if WITH_VECTOR_TEXTURES
void Renderer::DrawWallLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t clipLeft, uint8_t clipRight, uint8_t col)
{
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
	
	if (x1 < clipLeft)
	{
		if(x2 != x1)
		{
			y1 += ((clipLeft - x1) * (y2 - y1)) / (x2 - x1);
		}
		x1 = clipLeft;
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

	for (int x = x1; x <= x2 && x <= clipRight; x++)
	{
		int8_t horizon = horizonBuffer[x] - HORIZON;		

		Platform::PutPixel(x, horizon + y, col);

		yerror -= dy;

		while (yerror < 0)
		{
			y += ystep;
			
			//if(y < 0 || y >= DISPLAY_HEIGHT)
			//	return;
			
			yerror += dx;

			if (yerror < 0)
			{
				Platform::PutPixel(x, horizon + y, col);
			}

			if (x == x2 && y == y2)
				break;
		}
	}
}
#endif

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWallSegment(const uint16_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, bool shadeEdge)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWallSegment(const uint8_t* texture, int16_t x1, int16_t w1, int16_t x2, int16_t w2, uint8_t u1clip, uint8_t u2clip, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
void Renderer::DrawWallSegment(int16_t x1, int16_t w1, int16_t x2, int16_t w2, bool edgeLeft, bool edgeRight, bool shadeEdge)
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
#if WITH_IMAGE_TEXTURES
	uint8_t du = u2clip - u1clip;
	int16_t uerror = werror;
	uint8_t u = u1clip;
#endif

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
	
	uint8_t segmentClipLeft = (uint8_t) x1;
	uint8_t segmentClipRight = x2 < DISPLAY_WIDTH ? (uint8_t) x2 : DISPLAY_WIDTH - 1;

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

#if WITH_IMAGE_TEXTURES
			{
				uint8_t y1 = w > horizon ? 0 : horizon - w;
				uint8_t y2 = horizon + w > DISPLAY_HEIGHT ? DISPLAY_HEIGHT : horizon + w;

				DrawVLine(x, y1, y2, sliceMask);
				uint16_t textureData = pgm_read_word(&texture[u % 16]);
				const uint16_t wallSize = w * 2;
				uint16_t wallPos = y1 - (horizon - w);
				
				for (uint8_t y = y1; y < y2; y++)
				{
					uint8_t v = (16 * wallPos) / wallSize;
					uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);

					if ((textureData & mask) == 0)
					{
						Platform::PutPixel(x, y, 0);
					}

					wallPos++;
				}
			}
#else
			int8_t extent = w > 64 ? 64 : w;
			Platform::DrawVLine(x, horizon - extent, horizon + extent, sliceMask);
			Platform::PutPixel(x, horizon + extent, edgeColour);
			Platform::PutPixel(x, horizon - extent, edgeColour);
#endif
			
			if(wBuffer[x] == 0)
			{
				numBufferSlicesFilled++;
			}
			
			if (w > 255)
				wBuffer[x] = 255;
			else
				wBuffer[x] = (uint8_t)w;
		}
		else
		{
			if(x == segmentClipLeft)
			{
				segmentClipLeft++;
			}
			else if(x < segmentClipRight)
			{
				segmentClipRight = x;
				break;
			}
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
				Platform::PutPixel(x, horizon + w - 1, edgeColour);
				Platform::PutPixel(x, horizon - w, edgeColour);
			}
		}
		
		#if WITH_IMAGE_TEXTURES
		uerror -= du;
		
		while(uerror < 0)
		{
			u++;
			uerror += dx;
		}
		#endif
	}
	
	if(segmentClipLeft == segmentClipRight)
		return;

#if WITH_VECTOR_TEXTURES
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
		DrawWallLine(outU1, HORIZON - interpw1 + outV1, outU2, HORIZON - interpw2 + outV2, segmentClipLeft, segmentClipRight, edgeColour);
		//DrawWallLine(outU1, -interpw1 + outV1, outU2, -interpw2 + outV2, edgeColour);
	}
#endif
}

bool Renderer::isFrustrumClipped(int16_t x, int16_t y)
{
	if ((camera.clipCos * (x - camera.cellX) - camera.clipSin * (y - camera.cellY)) < -512)
		return true;
	if ((camera.clipSin * (x - camera.cellX) + camera.clipCos * (y - camera.cellY)) < -512)
		return true;

	return false;
}

void Renderer::TransformToViewSpace(int16_t x, int16_t y, int16_t& outX, int16_t& outY)
{
	int32_t relX = x - camera.x;
	int32_t relY = y - camera.y;
	outY = (int16_t)((camera.rotCos * relX) >> 8) - (int16_t)((camera.rotSin * relY) >> 8);
	outX = (int16_t)((camera.rotSin * relX) >> 8) + (int16_t)((camera.rotCos * relY) >> 8);
}

void Renderer::TransformToScreenSpace(int16_t viewX, int16_t viewZ, int16_t& outX, int16_t& outW)
{
	// apply perspective projection
	outX = (int16_t)((int32_t)viewX * NEAR_PLANE * CAMERA_SCALE / viewZ);
	outW = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ);

	// transform into screen space
	outX = (int16_t)((DISPLAY_WIDTH / 2) + outX);
}

#if WITH_IMAGE_TEXTURES
void Renderer::DrawWall(const uint16_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#elif WITH_VECTOR_TEXTURES
void Renderer::DrawWall(const uint8_t* texture, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#else
void Renderer::DrawWall(int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool edgeLeft, bool edgeRight, bool shadeEdge)
#endif
{
	int16_t viewX1, viewZ1, viewX2, viewZ2;
#if WITH_VECTOR_TEXTURES
	uint8_t u1 = 0, u2 = 128;
#elif WITH_IMAGE_TEXTURES
	uint8_t u1 = 0, u2 = 16;
#endif

	TransformToViewSpace(x1, y1, viewX1, viewZ1);
	TransformToViewSpace(x2, y2, viewX2, viewZ2);

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

	if (sx1 >= sx2 || sx2 <= 0 || sx1 >= DISPLAY_WIDTH)
		return;

	int16_t w1 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ1);
	int16_t w2 = (int16_t)((CELL_SIZE / 2 * NEAR_PLANE * CAMERA_SCALE) / viewZ2);

#if WITH_TEXTURES
	DrawWallSegment(texture, sx1, w1, sx2, w2, u1, u2, edgeLeft, edgeRight, shadeEdge);
#else
	DrawWallSegment(sx1, w1, sx2, w2, edgeLeft, edgeRight, shadeEdge);
#endif
}

void Renderer::DrawCell(uint8_t x, uint8_t y)
{
	CellType cellType = Map::GetCellSafe(x, y);
	
	switch(cellType)
	{
	case CellType::Skeleton:
		DrawObject(skeletonSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2);
		return;
	case CellType::Torch:
		{
			const uint16_t* torchSpriteData = globalAnimationFrame & 4 ? torchSpriteData1 : torchSpriteData2;
			
			if(Map::IsSolid(x - 1, y))
			{
				DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2);
			}
			else if(Map::IsSolid(x + 1, y))
			{
				DrawObject(torchSpriteData, x * CELL_SIZE + 6 * CELL_SIZE / 7, y * CELL_SIZE + CELL_SIZE / 2);
			}
			else if(Map::IsSolid(x, y - 1))
			{
				DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 7);
			}
			else if(Map::IsSolid(x, y + 1))
			{
				DrawObject(torchSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + 6 * CELL_SIZE / 7);
			}
		}
		return;
	case CellType::Exit:
		DrawObject(exitSpriteData, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2);
		return;
	}

	if(numBufferSlicesFilled >= DISPLAY_WIDTH)
	{
		return;
	}

	if (isFrustrumClipped(x, y))
	{
		return;
	}

	if (!Map::IsSolid(x, y))
	{
		return;
	}
	
	int16_t x1 = x * CELL_SIZE;
	int16_t y1 = y * CELL_SIZE;
	int16_t x2 = x1 + CELL_SIZE;
	int16_t y2 = y1 + CELL_SIZE;

	bool blockedLeft = Map::IsSolid(x - 1, y);
	bool blockedRight = Map::IsSolid(x + 1, y);
	bool blockedUp = Map::IsSolid(x, y - 1);
	bool blockedDown = Map::IsSolid(x, y + 1);

#if WITH_IMAGE_TEXTURES
	const uint16_t* texture = wallTextureData + (16 * (cellType - 1));
#elif WITH_VECTOR_TEXTURES
	const uint8_t* texture = (const uint8_t*) pgm_read_ptr(&textures[(uint8_t)cellType - 1]);
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

void Renderer::DrawCells()
{
	constexpr int8_t MAP_BUFFER_WIDTH = 16;
	constexpr int8_t MAP_BUFFER_HEIGHT = 16;
	
	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);

	int8_t bufferX = (int8_t)((camera.x + cosAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;
	int8_t bufferY = (int8_t)((camera.y + sinAngle * 7) >> 8) - MAP_BUFFER_WIDTH / 2;; 
	
	if(bufferX < 0)
		bufferX = 0;
	if(bufferY < 0)
		bufferY = 0;
	if(bufferX > Map::width - MAP_BUFFER_WIDTH)
		bufferX = Map::width - MAP_BUFFER_WIDTH;
	if(bufferY > Map::height - MAP_BUFFER_HEIGHT)
		bufferY = Map::height - MAP_BUFFER_HEIGHT;
	
	// This should make cells draw front to back
	
	int8_t xd, yd;
	int8_t x1, y1, x2, y2;

	if(camera.rotCos > 0)
	{
		x1 = bufferX;
		x2 = x1 + MAP_BUFFER_WIDTH;
		xd = 1;
	}
	else
	{
		x2 = bufferX - 1;
		x1 = x2 + MAP_BUFFER_WIDTH;
		xd = -1;
	}
	if(camera.rotSin < 0)
	{
		y1 = bufferY;
		y2 = y1 + MAP_BUFFER_HEIGHT;
		yd = 1;
	}
	else
	{
		y2 = bufferY - 1;
		y1 = y2 + MAP_BUFFER_HEIGHT;
		yd = -1;
	}

	if(ABS(camera.rotCos) < ABS(camera.rotSin))
	{
		for(int8_t y = y1; y != y2; y += yd)
		{
			for(int8_t x = x1; x != x2; x+= xd)
			{
				DrawCell(x, y);
			}
		}
	}
	else
	{
		for(int8_t x = x1; x != x2; x+= xd)
		{
			for(int8_t y = y1; y != y2; y += yd)
			{
				DrawCell(x, y);
			}
		}
	}	
}

template<int scaleMultiplier>
inline void DrawScaledOutline(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
	uint8_t size = 2 * halfSize;
	const uint8_t* lut = scaleLUT + ((halfSize / scaleMultiplier) * (halfSize / scaleMultiplier));

	uint8_t i0 = x < 0 ? -x : 0;
	uint8_t i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	uint8_t j0 = y < 0 ? -y : 0;
	uint8_t j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int8_t outX = x >= 0 ? x : 0;
	
	uint16_t leftTransparencyAndColourColumn = 0;
	uint16_t middleTransparencyColumn = 0;
	uint16_t rightTransparencyColumn = 0;
	uint16_t middleColourColumn = 0;
	uint16_t rightColourColumn = 0;
	bool wasVisible = false;

	for (uint8_t i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			const uint8_t u = pgm_read_byte(&lut[i / scaleMultiplier]);
			uint16_t leftRightOutlineColumn = 0;
			
			if(wasVisible)
			{
				leftTransparencyAndColourColumn = middleColourColumn & middleTransparencyColumn;		
				middleColourColumn = rightColourColumn;
				middleTransparencyColumn = rightTransparencyColumn;
				rightTransparencyColumn = pgm_read_word(&data[u * 2]);
				rightColourColumn = pgm_read_word(&data[u * 2 + 1]);
				leftRightOutlineColumn = leftTransparencyAndColourColumn | (rightColourColumn | rightTransparencyColumn);
			}
			else
			{
				leftTransparencyAndColourColumn = 0;		
				rightTransparencyColumn = pgm_read_word(&data[u * 2]);
				rightColourColumn = pgm_read_word(&data[u * 2 + 1]);
				middleColourColumn = rightColourColumn;
				middleTransparencyColumn = rightTransparencyColumn;
				leftRightOutlineColumn = (rightColourColumn | rightTransparencyColumn);
			}
			
			int8_t outY = y >= 0 ? y : 0;
			uint8_t bufferPos = (outY & 7);
			uint8_t* screenBuffer = Platform::GetScreenBuffer() + outX + ((outY & 0x38) << 4);
			uint8_t localBuffer = *screenBuffer;
			uint8_t writeMask = pgm_read_byte(&scaleDrawWriteMasks[bufferPos]);
			
			bool upIsOpaqueAndWhite = false;
			bool middleIsOpaque = false;
			bool downIsOpaque = false;
			bool middleIsWhite = false;
			bool downIsWhite = false;
			
			for (uint8_t j = j0; j < j1; j += scaleMultiplier)
			{
				uint8_t v = pgm_read_byte(&lut[j / scaleMultiplier]);
				uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);

				bool leftOrRightIsOutline = (leftRightOutlineColumn & mask) != 0;

				for(uint8_t k = 0; k < scaleMultiplier; k++)
				{
					upIsOpaqueAndWhite = middleIsOpaque && middleIsWhite;
					middleIsOpaque = downIsOpaque;
					middleIsWhite = downIsWhite;
					downIsOpaque = (middleTransparencyColumn & mask) != 0; 
					downIsWhite = (middleColourColumn & mask) != 0;
					
					if (middleIsOpaque)
					{
						if(middleIsWhite)
						{
							localBuffer |= writeMask;
						}
						else
						{
							localBuffer &= ~writeMask;
						}
					}
					else if(leftOrRightIsOutline || (upIsOpaqueAndWhite) || (downIsOpaque && downIsWhite))
					{
						localBuffer &= ~writeMask;
					}
					
					outY++;
					bufferPos++;
					writeMask <<= 1;
					
					if(bufferPos == 8)
					{
						bufferPos = 0;
						writeMask = 1;
						
						*screenBuffer = localBuffer;
						if(outY < DISPLAY_HEIGHT)
						{
							screenBuffer += 128;
						}
						localBuffer = *screenBuffer;
					}
				}
			}

			*screenBuffer = localBuffer;
		}

		outX++;
		wasVisible = isVisible;
	}
}

template<int scaleMultiplier>
inline void DrawScaledNoOutline(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
	uint8_t size = 2 * halfSize;
	const uint8_t* lut = scaleLUT + ((halfSize / scaleMultiplier) * (halfSize / scaleMultiplier));

	uint8_t i0 = x < 0 ? -x : 0;
	uint8_t i1 = x + size > DISPLAY_WIDTH ? DISPLAY_WIDTH - x : size;
	uint8_t j0 = y < 0 ? -y : 0;
	uint8_t j1 = y + size > DISPLAY_HEIGHT ? DISPLAY_HEIGHT - y : size;

	int8_t outX = x >= 0 ? x : 0;

	for (uint8_t i = i0; i < i1; i++)
	{
		const bool isVisible = Renderer::wBuffer[outX] < inverseCameraDistance;

		if (isVisible)
		{
			const uint8_t u = pgm_read_byte(&lut[i / scaleMultiplier]);
			int8_t outY = y >= 0 ? y : 0;
			uint8_t bufferPos = (outY & 7);
			uint8_t* screenBuffer = GetScreenBuffer() + outX + ((outY & 0x38) << 4);
			uint8_t localBuffer = *screenBuffer;
			uint8_t writeMask = pgm_read_byte(&scaleDrawWriteMasks[bufferPos]);
			uint16_t transparencyColumn = pgm_read_word(&data[u * 2]);
			uint16_t colourColumn = pgm_read_word(&data[u * 2 + 1]);

			for (uint8_t j = j0; j < j1; j += scaleMultiplier)
			{
				uint8_t v = pgm_read_byte(&lut[j / scaleMultiplier]);
				uint16_t mask = pgm_read_word(&scaleDrawReadMasks[v]);

				for (uint8_t k = 0; k < scaleMultiplier; k++)
				{
					bool isOpaque = (transparencyColumn & mask) != 0;

					if (isOpaque)
					{
						bool isWhite = (colourColumn & mask) != 0;

						if (isWhite)
						{
							localBuffer |= writeMask;
						}
						else
						{
							localBuffer &= ~writeMask;
						}
					}

					outY++;
					bufferPos++;
					writeMask <<= 1;

					if (bufferPos == 8)
					{
						bufferPos = 0;
						writeMask = 1;

						*screenBuffer = localBuffer;
						if (outY < DISPLAY_HEIGHT)
						{
							screenBuffer += 128;
						}
						localBuffer = *screenBuffer;
					}
				}
			}

			*screenBuffer = localBuffer;
		}

		outX++;
	}
}

void Renderer::DrawScaled(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
	uint8_t size = 2 * halfSize;

	if (size > MAX_SPRITE_SIZE * 4)
	{
		return;
	}
	else if (size > MAX_SPRITE_SIZE * 2)
	{
		DrawScaledInner<4>(data, x, y, halfSize, inverseCameraDistance);
	}
	else if (size > MAX_SPRITE_SIZE)
	{
		DrawScaledInner<2>(data, x, y, halfSize, inverseCameraDistance);
	}
	else if(halfSize > 2)
	{
		DrawScaledInner<1>(data, x, y, halfSize, inverseCameraDistance);
	}
	else if (halfSize == 2)
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
		{
			Platform::PutPixel(x, y, COLOUR_BLACK);
			Platform::PutPixel(x, y + 1, COLOUR_BLACK);
		}
		if (Renderer::wBuffer[x + 1] < inverseCameraDistance)
		{
			Platform::PutPixel(x + 1, y, COLOUR_BLACK);
			Platform::PutPixel(x + 1, y + 1, COLOUR_BLACK);
		}
	}
	else
	{
		if (Renderer::wBuffer[x] < inverseCameraDistance)
		{
			Platform::PutPixel(x, y, COLOUR_BLACK);
		}
	}
}

QueuedDrawable* Renderer::CreateQueuedDrawable(uint8_t inverseCameraDistance)
{
	uint8_t insertionPoint = MAX_QUEUED_DRAWABLES;
	
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		if(inverseCameraDistance < queuedDrawables[n].inverseCameraDistance)
		{
			if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
			{
				insertionPoint = n;
				numQueuedDrawables++;
				
				for (uint8_t i = numQueuedDrawables - 1; i > n; i--)
				{
					queuedDrawables[i] = queuedDrawables[i - 1];
				}
			}
			else
			{
				if(n == 0)
				{
					// List is full and this is smaller than the first element so just cull
					return nullptr;
				}
				
				// Drop the smallest element to make a space
				for (uint8_t i = 0; i < n - 1; i++)
				{
					queuedDrawables[i] = queuedDrawables[i + 1];
				}
				
				insertionPoint = n - 1;
			}
			
			break;
		}
	}
	
	if(insertionPoint == MAX_QUEUED_DRAWABLES)
	{
		if(numQueuedDrawables < MAX_QUEUED_DRAWABLES)
		{
			insertionPoint = numQueuedDrawables;
			numQueuedDrawables++;
		}
		else if (inverseCameraDistance > queuedDrawables[numQueuedDrawables - 1].inverseCameraDistance)
		{
			// Drop the smallest element to make a space
			for (uint8_t i = 0; i < numQueuedDrawables - 1; i++)
			{
				queuedDrawables[i] = queuedDrawables[i + 1];
			}
			insertionPoint = numQueuedDrawables - 1;
		}
		else
		{
			return nullptr;
		}
	}
	
	return &queuedDrawables[insertionPoint];
}

void Renderer::QueueSprite(const uint16_t* data, int8_t x, int8_t y, uint8_t halfSize, uint8_t inverseCameraDistance)
{
	if(x < -halfSize * 2)
		return;
	if(x >= DISPLAY_WIDTH)
		return;
	//if(halfSize <= 2)
	//	return;

	QueuedDrawable* drawable = CreateQueuedDrawable(inverseCameraDistance);
	
	if(drawable != nullptr)
	{
		drawable->type = DrawableType::Sprite;
		drawable->spriteData = data;
		drawable->x = x;
		drawable->y = y;
		drawable->halfSize = halfSize;
		drawable->inverseCameraDistance = inverseCameraDistance;
	}
}

void Renderer::RenderQueuedDrawables()
{
	for(uint8_t n = 0; n < numQueuedDrawables; n++)
	{
		QueuedDrawable& drawable = queuedDrawables[n];
		
		if(drawable.type == DrawableType::Sprite)
		{
			DrawScaled(drawable.spriteData, drawable.x, drawable.y, drawable.halfSize, drawable.inverseCameraDistance);
		}
		else
		{
			drawable.particleSystem->Draw(drawable.x, drawable.inverseCameraDistance);
		}
	}
}

int8_t Renderer::GetHorizon(int16_t x)
{
	if (x < 0)
		x = 0;
	if (x >= DISPLAY_WIDTH)
		x = DISPLAY_WIDTH - 1;
	return horizonBuffer[x];
}

bool Renderer::TransformAndCull(int16_t worldX, int16_t worldY, int16_t& outScreenX, int16_t& outScreenW)
{
	int16_t relX, relZ;
	TransformToViewSpace(worldX, worldY, relX, relZ);

	// Frustum cull
	if (relZ < CLIP_PLANE)
		return false;

	if (relX < 0 && -2 * relZ > relX)
		return false;
	if (relX > 0 && 2 * relZ < relX)
		return false;

	TransformToScreenSpace(relX, relZ, outScreenX, outScreenW);
	
	return true;
}

void Renderer::DrawObject(const uint16_t* spriteData, int16_t x, int16_t y)
{
	int16_t screenX, screenW;

	if(TransformAndCull(x, y, screenX, screenW))
	{
		// Bit of a hack: nudge sorting closer to the camera
		uint8_t inverseCameraDistance = (uint8_t)(screenW + 1);
		
		if(spriteData == projectileSpriteData)
		{
			int16_t spriteSize = screenW / 4;
			QueueSprite(spriteData, screenX - spriteSize, GetHorizon(screenX) - spriteSize / 2, (uint8_t) spriteSize, inverseCameraDistance);
		}
		else if(spriteData == skeletonSpriteData)
		{
			int16_t spriteSize = (3 * screenW) / 4;
			QueueSprite(spriteData, screenX - spriteSize, GetHorizon(screenX) - screenW / 4, (uint8_t) spriteSize, inverseCameraDistance);
		}
		else if(spriteData == exitSpriteData)
		{
			int16_t spriteSize = (3 * screenW) / 4;
			QueueSprite(spriteData, screenX - spriteSize, GetHorizon(screenX) - screenW / 4, (uint8_t) spriteSize, inverseCameraDistance);
		}
		else
		{
			int16_t spriteSize = (screenW) / 2;
			QueueSprite(spriteData, screenX - spriteSize, GetHorizon(screenX) - spriteSize, (uint8_t) spriteSize, inverseCameraDistance);
		}
	}
}

void Renderer::DrawWeapon()
{
	int x = DISPLAY_WIDTH / 2 + 22 + camera.tilt / 4;
	int y = DISPLAY_HEIGHT - 21 - camera.bob;
	uint8_t reloadTime = Game::player.reloadTime;
	
	if(reloadTime > 0)
	{
		Platform::DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, handSpriteData2, 0);
		//DrawSprite(x - reloadTime / 3 - 1, y - reloadTime / 3 - 1, handSpriteData2, handSpriteData2_mask, 0, 0);	
	}
	else
	{
		Platform::DrawSprite(x + 2, y + 2, handSpriteData1, 0);
		//DrawSprite(x + 2, y + 2, handSpriteData1, handSpriteData1_mask, 0, 0);	
	}
	
}

void ProjectileManager::Draw()
{
	for(uint8_t n = 0; n < MAX_PROJECTILES; n++)
	{
		Projectile& p = projectiles[n];
		if(p.life > 0)
		{
			Renderer::DrawObject(projectileSpriteData, p.x, p.y);
		}
	}	
}

void Renderer::DrawBackground()
{
	uint8_t* ptr = Platform::GetScreenBuffer();
	uint8_t counter = 64;

	while (counter--)
	{
		*ptr++ = 0x55;  *ptr++ = 0;
		*ptr++ = 0x55;  *ptr++ = 0;
		*ptr++ = 0x55;  *ptr++ = 0;
		*ptr++ = 0x55;  *ptr++ = 0;
	}

	counter = 64;
	while (counter--)
	{
		*ptr++ = 0x55;  *ptr++ = 0xAA;
		*ptr++ = 0x55;  *ptr++ = 0xAA;
		*ptr++ = 0x55;  *ptr++ = 0xAA;
		*ptr++ = 0x55;  *ptr++ = 0xAA;
	}
}

void Renderer::Render()
{
	DrawBackground();

	globalAnimationFrame++;
	numBufferSlicesFilled = 0;
	numQueuedDrawables = 0;
	
	for (uint8_t n = 0; n < DISPLAY_WIDTH; n++)
	{
		wBuffer[n] = 0;
		horizonBuffer[n] = HORIZON + (((DISPLAY_WIDTH / 2 - n) * camera.tilt) >> 8) + camera.bob;
	}

	camera.cellX = camera.x / CELL_SIZE;
	camera.cellY = camera.y / CELL_SIZE;

	camera.rotCos = FixedCos(-camera.angle);
	camera.rotSin = FixedSin(-camera.angle);
	camera.clipCos = FixedCos(-camera.angle + CLIP_ANGLE);
	camera.clipSin = FixedSin(-camera.angle + CLIP_ANGLE);

	DrawCells();

	ProjectileManager::Draw();
	ParticleSystemManager::Draw();
	
	RenderQueuedDrawables();
	
	DrawWeapon();
}

