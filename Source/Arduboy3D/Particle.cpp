#include "Particle.h"
#include "FixedMath.h"

void ParticleSystem::Init()
{
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		particles[n].x = -128;
	}
}

void ParticleSystem::Step()
{
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];

		if (p.IsActive())
		{
			p.velY += gravity;
			p.life--;

			if (p.x + p.velX < -127 || p.x + p.velX > 127 || p.y + p.velY < -127 || p.life == 0)
			{
				p.x = -128;
				continue;
			}

			if (p.y + p.velY >= 128)
			{
				p.velY = p.velX = 0;
				p.y = 127;
			}

			p.x += p.velX;
			p.y += p.velY;

			//if(p.y > 64)
			//{
			//	p.y = 64;
			//}
		}
	}
}

void ParticleSystem::Draw(int x, int halfScale)
{
	int scale = 2 * halfScale;
	int8_t horizon = GetHorizon(x);
	
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];

		if (p.IsActive())
		{
			//int outX = x + ((p.x * scale) >> 8);
			//int outY = HORIZON + ((p.y * scale) >> 8);
			int outX = x + ((p.x * scale) >> 8);
			int outY = horizon + ((p.y * scale) >> 8);

			if (outX >= 0 && outY >= 0 && outX < DISPLAY_WIDTH - 1 && outY < DISPLAY_HEIGHT - 1 && halfScale >= wBuffer[outX])
			{
				PutPixel(outX, outY, COLOUR_BLACK);
				PutPixel(outX + 1, outY, COLOUR_BLACK);
				PutPixel(outX + 1, outY + 1, COLOUR_BLACK);
				PutPixel(outX, outY + 1, COLOUR_BLACK);
			}
		}
	}
}

void ParticleSystem::Explode(uint8_t count)
{
	bool searchExhausted = false;

	for (int n = 0; n < PARTICLES_PER_SYSTEM && count; n++)
	{
		Particle& p = particles[n];

		if (searchExhausted || !p.IsActive())
		{
			p.x = (Random() & 31) - 16;
			p.y = (Random() & 31) - 16;

			p.velX = (Random() & 31) - 16;
			p.velY = (Random() & 31) - 25;

			p.life = (Random() & 15) + 6;
			count--;
		}

		if (n == PARTICLES_PER_SYSTEM - 1 && !searchExhausted)
		{
			searchExhausted = true;
			n = 0;
		}
	}
}
