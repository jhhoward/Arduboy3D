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
	static constexpr int8_t gravity = 3;
	int16_t worldX, worldY;
	bool isActive : 1;
	Particle particles[PARTICLES_PER_SYSTEM];

	void Init();
	void Step();
	void Draw(int x, int scale);
	void Explode(uint8_t count);
};

class ParticleSystemManager
{
public:
	static constexpr int MAX_SYSTEMS = 3;
	static ParticleSystem systems[MAX_SYSTEMS];
	
	static void Draw();
	static void Update();
	static void CreateExplosion(int16_t x, int16_t y);
};
