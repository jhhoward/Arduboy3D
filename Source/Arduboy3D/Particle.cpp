#include "Particle.h"
#include "FixedMath.h"
#include "Platform.h"

ParticleSystem ParticleSystemManager::systems[MAX_SYSTEMS];

void ParticleSystem::Init()
{
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		particles[n].x = -128;
	}
}

void ParticleSystem::Step()
{
	isActive = false;
	
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];

		if (p.life > 0)
		{
			p.velY += gravity;
			p.life--;

			if (p.x + p.velX < -127 || p.x + p.velX > 127 || p.y + p.velY < -127 || p.life == 0)
			{
				p.life = 0;
				continue;
			}

			if (p.y + p.velY >= 128)
			{
				p.velY = p.velX = 0;
				p.y = 127;
			}

			p.x += p.velX;
			p.y += p.velY;

			isActive = true;
		}
	}
}

void ParticleSystem::Draw(int x, int halfScale)
{
	int scale = 2 * halfScale;
	int8_t horizon = Renderer::GetHorizon(x);
	uint8_t colour = isWhite ? COLOUR_WHITE : COLOUR_BLACK;
	
	for (int n = 0; n < PARTICLES_PER_SYSTEM; n++)
	{
		Particle& p = particles[n];

		if (p.life > 0)
		{
			//int outX = x + ((p.x * scale) >> 8);
			//int outY = HORIZON + ((p.y * scale) >> 8);
			int outX = x + ((p.x * scale) >> 8);
			int outY = horizon + ((p.y * scale) >> 8);

			if (outX >= 0 && outY >= 0 && outX < DISPLAY_WIDTH - 1 && outY < DISPLAY_HEIGHT - 1 && halfScale >= Renderer::wBuffer[outX])
			{
				Platform::PutPixel(outX, outY, colour);
				Platform::PutPixel(outX + 1, outY, colour);
				Platform::PutPixel(outX + 1, outY + 1, colour);
				Platform::PutPixel(outX, outY + 1, colour);
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

void ParticleSystemManager::Draw()
{
	for(uint8_t n = 0; n < MAX_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		
		if(system.isActive)
		{
			int16_t screenX, screenW;

			if(Renderer::TransformAndCull(system.worldX, system.worldY, screenX, screenW))
			{
				QueuedDrawable* drawable = Renderer::CreateQueuedDrawable((uint8_t)screenW);
				if(drawable)
				{
					drawable->type = DrawableType::ParticleSystem;
					drawable->x = (int8_t)screenX;
					drawable->inverseCameraDistance = (uint8_t)screenW;
					drawable->particleSystem = &system;
				}
			}
		}
	}
}

void ParticleSystemManager::Update()
{
	for(uint8_t n = 0; n < MAX_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		
		if(system.isActive)
		{
			system.Step();
		}
	}	
}

void ParticleSystemManager::CreateExplosion(int16_t worldX, int16_t worldY, bool isWhite)
{
	for(uint8_t n = 0; n < MAX_SYSTEMS; n++)
	{
		ParticleSystem& system = systems[n];
		
		if(!system.isActive)
		{
			system.worldX = worldX;
			system.worldY = worldY;
			system.isActive = true;
			system.isWhite = isWhite;
			system.Explode(PARTICLES_PER_SYSTEM);
			
			return;
		}
	}	
}
