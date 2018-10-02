#include "Defines.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"

void InitGame()
{
	camera.x = CELL_SIZE * 4;
	camera.y = CELL_SIZE * 4;

}

void TickGame()
{
	uint8_t input = GetInput();

	if (input & INPUT_LEFT)
	{
		camera.angle -= TURN_SPEED;
	}
	if (input & INPUT_RIGHT)
	{
		camera.angle += TURN_SPEED;
	}

	int8_t moveDelta = 0;
	if (input & INPUT_UP)
	{
		moveDelta++;
	}
	if (input & INPUT_DOWN)
	{
		moveDelta--;
	}

	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);
	camera.x += (moveDelta * cosAngle) >> 4;
	camera.y += (moveDelta * sinAngle) >> 4;

	Render();
}