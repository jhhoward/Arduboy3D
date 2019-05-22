#include "Defines.h"
#include "Projectile.h"
#include "Map.h"
#include "FixedMath.h"
#include "Particle.h"

Projectile ProjectileManager::projectiles[ProjectileManager::MAX_PROJECTILES];

void ProjectileManager::FireProjectile(int16_t x, int16_t y, uint8_t angle)
{
	for(uint8_t n = 0; n < MAX_PROJECTILES; n++)
	{
		Projectile& p = projectiles[n];
		if(p.life == 0)
		{
			p.life = 255;
			p.x = x;
			p.y = y;
			p.angle = angle;
			return;
		}
	}
}

void ProjectileManager::Update()
{
	for(uint8_t n = 0; n < MAX_PROJECTILES; n++)
	{
		Projectile& p = projectiles[n];
		if(p.life > 0)
		{
			p.life--;

			int16_t deltaX = FixedCos(p.angle) / 4;
			int16_t deltaY = FixedSin(p.angle) / 4;

			MoveResult moveResult = p.Move(deltaX, deltaY);

			if(moveResult.didCollide)
			{
				ParticleSystemManager::CreateExplosion(p.x - deltaX, p.y - deltaY);
				p.life = 0;
			}
		}
	}	
}