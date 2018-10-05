#pragma once

#include <stdint.h>
#include "Defines.h"

void InitGame(void);
void TickGame(void);

// Implemented by platform
uint8_t GetInput(void);
void PutPixel(uint8_t x, uint8_t y, uint8_t colour);
void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap);
void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap,
	const uint8_t *mask, uint8_t frame, uint8_t mask_frame);
void SetLED(uint8_t r, uint8_t g, uint8_t b);
void FillScreen(uint8_t col);
void DrawBackground(void);
void DrawVLine(uint8_t x, int8_t y1, int8_t y2, uint8_t pattern);

//


