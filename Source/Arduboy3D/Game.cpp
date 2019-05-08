#include "Defines.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Map.h"

#define USE_ROTATE_BOB 0
#define STRAFE_TILT 14
#define ROTATE_TILT 3

int16_t cameraVelocityX;
int16_t cameraVelocityY;

void InitGame()
{
	camera.x = CELL_SIZE * 1 + CELL_SIZE / 2;
	camera.y = CELL_SIZE * 1 + CELL_SIZE / 2;
}

int8_t cameraAngularVelocity = 0;

#define COLLISION_SIZE 48

bool IsObjectColliding(Camera* obj)
{
	return IsBlockedAtWorldPosition(obj->x - COLLISION_SIZE, obj->y - COLLISION_SIZE)
		|| IsBlockedAtWorldPosition(obj->x + COLLISION_SIZE, obj->y - COLLISION_SIZE)
		|| IsBlockedAtWorldPosition(obj->x + COLLISION_SIZE, obj->y + COLLISION_SIZE)
		|| IsBlockedAtWorldPosition(obj->x - COLLISION_SIZE, obj->y + COLLISION_SIZE);
}

void MoveCamera(Camera* entity, int16_t deltaX, int16_t deltaY)
{
	entity->x += deltaX;
	entity->y += deltaY;

	if (IsObjectColliding(entity))
	{
		entity->y -= deltaY;
		if (IsObjectColliding(entity))
		{
			entity->x -= deltaX;
			entity->y += deltaY;

			if (IsObjectColliding(entity))
			{
				entity->y -= deltaY;
			}
		}
	}
	/*int16_t newX = entity->x + deltaX;
	int16_t newY = entity->y + deltaY;

	if (deltaX < 0)
	{
		if (IsBlockedAtWorldPosition(newX - COLLISION_SIZE, entity->y)
			|| IsBlockedAtWorldPosition(newX - COLLISION_SIZE, entity->y - COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(newX - COLLISION_SIZE, entity->y + COLLISION_SIZE))
		{
			entity->x = (entity->x & 0xff00) + COLLISION_SIZE;
		}
		else
		{
			entity->x = newX;
		}
	}
	else if(deltaX > 0)
	{
		if (IsBlockedAtWorldPosition(newX + COLLISION_SIZE, entity->y)
			|| IsBlockedAtWorldPosition(newX + COLLISION_SIZE, entity->y - COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(newX + COLLISION_SIZE, entity->y + COLLISION_SIZE))
		{
			entity->x = (entity->x & 0xff00) + CELL_SIZE - COLLISION_SIZE;
		}
		else
		{
			entity->x = newX;
		}
	}

	if (deltaY < 0)
	{
		if (IsBlockedAtWorldPosition(entity->x, newY - COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(entity->x + COLLISION_SIZE, newY - COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(entity->x - COLLISION_SIZE, newY - COLLISION_SIZE))
		{
			entity->y = (entity->y & 0xff00) + COLLISION_SIZE;
		}
		else
		{
			entity->y = newY;
		}
	}
	else if (deltaY > 0)
	{
		if (IsBlockedAtWorldPosition(entity->x, newY + COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(entity->x + COLLISION_SIZE, newY + COLLISION_SIZE)
			|| IsBlockedAtWorldPosition(entity->x - COLLISION_SIZE, newY + COLLISION_SIZE))
		{
			entity->y = (entity->y & 0xff00) + CELL_SIZE - COLLISION_SIZE;
		}
		else
		{
			entity->y = newY;
		}
	}*/

}

void TickGame()
{
	uint8_t input = GetInput();
	int8_t turnDelta = 0;
	int8_t targetTilt = 0;
	int8_t moveDelta = 0;
	int8_t strafeDelta = 0;
	static uint8_t shakeTime = 0;
	static uint8_t reloadTime = 0;

	if(input & INPUT_A)
	{
		if (input & INPUT_LEFT)
		{
			strafeDelta--;
		}
		if (input & INPUT_RIGHT)
		{
			strafeDelta++;
		}
	}
	else
	{
		if (input & INPUT_LEFT)
		{
			turnDelta -= TURN_SPEED * 2;
		}
		if (input & INPUT_RIGHT)
		{
			turnDelta += TURN_SPEED * 2;
		}
	}

	if (reloadTime > 0)
	{
		reloadTime--;
	}
	else if (input & INPUT_B)
	{
		reloadTime = 13;
		shakeTime = 8;
		if(!moveDelta)
			moveDelta -= 5;
	}

	if (cameraAngularVelocity < turnDelta)
	{
		cameraAngularVelocity++;
	}
	else if (cameraAngularVelocity > turnDelta)
	{
		cameraAngularVelocity--;
	}

	camera.angle += cameraAngularVelocity >> 1;

	if (input & INPUT_UP)
	{
		moveDelta++;
	}
	if (input & INPUT_DOWN)
	{
		moveDelta--;
	}

	static int tiltTimer = 0;
	tiltTimer++;
	if (moveDelta && USE_ROTATE_BOB)
	{
		targetTilt = (int8_t)(FixedSin(tiltTimer * 10) / 32);
	}
	else
	{
		targetTilt = 0;
	}

	targetTilt += cameraAngularVelocity * ROTATE_TILT;
	targetTilt += strafeDelta * STRAFE_TILT;
	int8_t targetBob = moveDelta || strafeDelta ? FixedSin(tiltTimer * 10) / 128 : 0;

	if (shakeTime > 0)
	{
		shakeTime--;
		targetBob += (Random() & 3) - 1;
		targetTilt += (Random() & 31) - 16;
	}

	constexpr int tiltRate = 6;

	if (camera.tilt < targetTilt)
	{
		camera.tilt += tiltRate;
		if (camera.tilt > targetTilt)
		{
			camera.tilt = targetTilt;
		}
	}
	else if (camera.tilt > targetTilt)
	{
		camera.tilt -= tiltRate;
		if (camera.tilt < targetTilt)
		{
			camera.tilt = targetTilt;
		}
	}

	constexpr int bobRate = 3;

	if (camera.bob < targetBob)
	{
		camera.bob += bobRate;
		if (camera.bob > targetBob)
		{
			camera.bob = targetBob;
		}
	}
	else if (camera.bob > targetBob)
	{
		camera.bob -= bobRate;
		if (camera.bob < targetBob)
		{
			camera.bob = targetBob;
		}
	}

	int16_t cosAngle = FixedCos(camera.angle);
	int16_t sinAngle = FixedSin(camera.angle);

	int16_t cos90Angle = FixedCos(camera.angle + FIXED_ANGLE_90);
	int16_t sin90Angle = FixedSin(camera.angle + FIXED_ANGLE_90);
	//camera.x += (moveDelta * cosAngle) >> 4;
	//camera.y += (moveDelta * sinAngle) >> 4;
	cameraVelocityX += (moveDelta * cosAngle) / 32;
	cameraVelocityY += (moveDelta * sinAngle) / 32;

	cameraVelocityX += (strafeDelta * cos90Angle) / 32;
	cameraVelocityY += (strafeDelta * sin90Angle) / 32;
	
	cameraVelocityX = (cameraVelocityX * 7) / 8;
	cameraVelocityY = (cameraVelocityY * 7) / 8;

	MoveCamera(&camera, cameraVelocityX / 4, cameraVelocityY / 4);
	
	Render();
}