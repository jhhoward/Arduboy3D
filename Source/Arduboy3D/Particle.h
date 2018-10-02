#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Draw.h"
#include "Game.h"

struct Particle
{
	int8_t x, y;
	int8_t velX, velY;
	uint8_t life;

	inline bool IsActive() { return x != -128; }
};

struct ParticleSystem
{
	const int8_t gravity = 3;
	Particle particles[PARTICLES_PER_SYSTEM];

	void Init();
	void Step();
	void Draw(int x, int scale);
	void Explode(uint8_t count);
};
