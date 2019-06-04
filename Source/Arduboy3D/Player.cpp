#include "Player.h"
#include "Game.h"
#include "FixedMath.h"
#include "Projectile.h"
#include "Platform.h"
#include "Draw.h"
#include "Enemy.h"
#include "Map.h"

#define USE_ROTATE_BOB 0
#define STRAFE_TILT 14
#define ROTATE_TILT 3

void Player::Init()
{
	x = CELL_SIZE * 1 + CELL_SIZE / 2;
	y = CELL_SIZE * 1 + CELL_SIZE / 2;
	angle = FIXED_ANGLE_45;
	hp = maxHP;
	mana = maxMana;
	damageTime = 0;
	shakeTime = 0;
	reloadTime = 0;
}

void Player::Fire()
{
	if (mana >= manaFireCost)
	{
		reloadTime = 8;
		shakeTime = 6;

		int16_t projectileX = x + FixedCos(angle + FIXED_ANGLE_90 / 2) / 4;
		int16_t projectileY = y + FixedSin(angle + FIXED_ANGLE_90 / 2) / 4;

		ProjectileManager::FireProjectile(this, projectileX, projectileY, angle);
		mana -= manaFireCost;
	}
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

	if (mana < maxMana && reloadTime == 0)
	{
		mana += manaRechargeRate;
	}

	if (damageTime > 0)
		damageTime--;

	uint8_t cellX = x / CELL_SIZE;
	uint8_t cellY = y / CELL_SIZE;

	switch (Map::GetCellSafe(cellX, cellY))
	{
	case CellType::Potion:
		Map::SetCell(cellX, cellY, CellType::Empty);
		hp = maxHP;
		break;
	}
}

bool Player::CheckCollisions()
{
	if (IsWorldColliding())
	{
		return true;
	}

	if (EnemyManager::GetOverlappingEnemy(*this))
	{
		return true;
	}

	return false;
}

void Player::Move(int16_t deltaX, int16_t deltaY)
{
	x += deltaX;
	y += deltaY;

	if (CheckCollisions())
	{
		y -= deltaY;
		if (CheckCollisions())
		{
			x -= deltaX;
			y += deltaY;

			if (CheckCollisions())
			{
				y -= deltaY;
			}
		}
	}
}

void Player::Damage()
{
	uint8_t damageAmount = 10;

	if(shakeTime < 6)
		shakeTime = 6;

	damageTime = 8;
	
	if (hp <= damageAmount)
	{
		Die();
	}
	else
	{
		hp -= damageAmount;
	}
}

void Player::Die()
{
	// TODO
	hp = 0;
}