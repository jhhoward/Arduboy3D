#pragma once

#include <stdint.h>

struct Projectile
{
	int16_t x, y;
	uint8_t angle;
	uint8_t life;
};

class ProjectileManager
{
public:
	static constexpr int MAX_PROJECTILES = 4;
	static Projectile projectiles[MAX_PROJECTILES];

	static void FireProjectile(int16_t x, int16_t y, uint8_t angle);
	static void Draw();
	static void Update();
};
