#include "Defines.h"
#include "Projectile.h"
#include "Map.h"
#include "FixedMath.h"
#include "Particle.h"
#include "Enemy.h"

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

			p.x += deltaX;
			p.y += deltaY;

			bool hitAnything = false;

			Enemy* overlappingEnemy = EnemyManager::GetOverlappingEnemy(p);
			if (overlappingEnemy)
			{
				overlappingEnemy->Damage();
				ParticleSystemManager::CreateExplosion(p.x, p.y, true);

				hitAnything = true;
			}
			else if (Map::IsBlockedAtWorldPosition(p.x, p.y))
			{
				uint8_t cellX = p.x / CELL_SIZE;
				uint8_t cellY = p.y / CELL_SIZE;

				if (Map::GetCellSafe(cellX, cellY) == CellType::Urn)
				{
					Map::SetCell(cellX, cellY, CellType::Empty);
					ParticleSystemManager::CreateExplosion(cellX * CELL_SIZE + CELL_SIZE / 2, cellY * CELL_SIZE + CELL_SIZE / 2, true);
				}

				hitAnything = true;
			}

			if (hitAnything)
			{
				ParticleSystemManager::CreateExplosion(p.x - deltaX, p.y - deltaY);

				p.life = 0;
			}
		}
	}	
}