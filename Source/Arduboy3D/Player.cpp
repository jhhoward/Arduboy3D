#include "Player.h"
#include "Game.h"
#include "FixedMath.h"
#include "Projectile.h"
#include "Platform.h"
#include "Draw.h"

#define USE_ROTATE_BOB 0
#define STRAFE_TILT 14
#define ROTATE_TILT 3

void Player::Fire()
{
	reloadTime = 8;
	shakeTime = 6;

	int16_t projectileX = x + FixedCos(angle + FIXED_ANGLE_90 / 2) / 4;
	int16_t projectileY = y + FixedSin(angle + FIXED_ANGLE_90 / 2) / 4;

	ProjectileManager::FireProjectile(projectileX, projectileY, angle);
}

void Player::Tick()
{
	uint8_t input = Platform::GetInput();
	int8_t turnDelta = 0;
	int8_t targetTilt = 0;
	int8_t moveDelta = 0;
	int8_t strafeDelta = 0;

	if (input & INPUT_A)
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

	// Testing shooting / recoil mechanic

	if (reloadTime > 0)
	{
		reloadTime--;
	}
	else if (input & INPUT_B)
	{
		Fire();
	}


	if (angularVelocity < turnDelta)
	{
		angularVelocity++;
	}
	else if (angularVelocity > turnDelta)
	{
		angularVelocity--;
	}

	angle += angularVelocity >> 1;

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

	targetTilt += angularVelocity * ROTATE_TILT;
	targetTilt += strafeDelta * STRAFE_TILT;
	int8_t targetBob = moveDelta || strafeDelta ? FixedSin(tiltTimer * 10) / 128 : 0;

	if (shakeTime > 0)
	{
		shakeTime--;
		targetBob += (Random() & 3) - 1;
		targetTilt += (Random() & 31) - 16;
	}

	constexpr int tiltRate = 6;

	if (Renderer::camera.tilt < targetTilt)
	{
		Renderer::camera.tilt += tiltRate;
		if (Renderer::camera.tilt > targetTilt)
		{
			Renderer::camera.tilt = targetTilt;
		}
	}
	else if (Renderer::camera.tilt > targetTilt)
	{
		Renderer::camera.tilt -= tiltRate;
		if (Renderer::camera.tilt < targetTilt)
		{
			Renderer::camera.tilt = targetTilt;
		}
	}

	constexpr int bobRate = 3;

	if (Renderer::camera.bob < targetBob)
	{
		Renderer::camera.bob += bobRate;
		if (Renderer::camera.bob > targetBob)
		{
			Renderer::camera.bob = targetBob;
		}
	}
	else if (Renderer::camera.bob > targetBob)
	{
		Renderer::camera.bob -= bobRate;
		if (Renderer::camera.bob < targetBob)
		{
			Renderer::camera.bob = targetBob;
		}
	}

	int16_t cosAngle = FixedCos(angle);
	int16_t sinAngle = FixedSin(angle);

	int16_t cos90Angle = FixedCos(angle + FIXED_ANGLE_90);
	int16_t sin90Angle = FixedSin(angle + FIXED_ANGLE_90);
	//camera.x += (moveDelta * cosAngle) >> 4;
	//camera.y += (moveDelta * sinAngle) >> 4;
	velocityX += (moveDelta * cosAngle) / 24;
	velocityY += (moveDelta * sinAngle) / 24;

	velocityX += (strafeDelta * cos90Angle) / 24;
	velocityY += (strafeDelta * sin90Angle) / 24;

	Move(velocityX / 4, velocityY / 4);

	velocityX = (velocityX * 7) / 8;
	velocityY = (velocityY * 7) / 8;
}
