#pragma once


struct Camera
{
	int16_t x, y;
	uint8_t angle;
	int16_t rotCos, rotSin;
	int16_t clipCos, clipSin;
	uint8_t cellX, cellY;
	int8_t tilt;
	int8_t bob;
};

extern uint8_t wBuffer[];
extern Camera camera;

void Render(void);
int8_t GetHorizon(int16_t x);
