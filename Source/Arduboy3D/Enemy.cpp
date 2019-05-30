#include "Enemy.h"
#include "Defines.h"
#include "Draw.h"
#include "Map.h"
#include "FixedMath.h"
#include "Game.h"

Enemy EnemyManager::enemies[maxEnemies];

void Enemy::Init(EnemyType initType, int16_t initX, int16_t initY)
{
	type = initType;
	x = initX;
	y = initY;
	targetCellX = x / CELL_SIZE;
	targetCellY = y / CELL_SIZE;
}

int16_t Clamp(int16_t x, int16_t min, int16_t max)
{
	if(x < min)
		return min;
	if(x > max)
		return max;
	return x;
}

bool Enemy::TryPickCell(int8_t newX, int8_t newY)
{
	if(Map::IsBlocked(newX, newY))// && !engine.map.isDoor(newX, newZ))
		return false;
	if(Map::IsBlocked(targetCellX, newY)) // && !engine.map.isDoor(targetCellX, newZ))
		return false;
	if(Map::IsBlocked(newX, targetCellY)) // && !engine.map.isDoor(newX, targetCellZ))
		return false;

	//for(int n = 0; n < MAX_ACTIVE_ACTORS; n++)
	//{
	//	if(this != &engine.actors[n] && engine.actors[n].type != ActorType_Empty && engine.actors[n].hp > 0)
	//	{
	//		if(engine.actors[n].targetCellX == newX && engine.actors[n].targetCellZ == newZ)
	//			return false;
	//	}
	//}

	targetCellX = newX;
	targetCellY = newY;

	return true;
}

bool Enemy::TryPickCells(int8_t deltaX, int8_t deltaY)
{
	return TryPickCell(targetCellX + deltaX, targetCellY + deltaY)
		|| TryPickCell(targetCellX + deltaX, targetCellY) 
		|| TryPickCell(targetCellX, targetCellY + deltaY) 
		|| TryPickCell(targetCellX - deltaX, targetCellY + deltaY)
		|| TryPickCell(targetCellX + deltaX, targetCellY - deltaY);
}

void Enemy::PickNewTargetCell()
{
	int8_t deltaX = (int8_t) Clamp((Game::player.x / CELL_SIZE) - targetCellX, -1, 1);
	int8_t deltaY = (int8_t) Clamp((Game::player.y / CELL_SIZE) - targetCellY, -1, 1);
	uint8_t dodgeChance = (uint8_t) Random();

	if(deltaX == 0)
	{
		if(dodgeChance < 64)
		{
			deltaX = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaX = 1;
		}
	}
	else if(deltaY == 0)
	{
		if(dodgeChance < 64)
		{
			deltaY = -1;
		}
		else if(dodgeChance < 128)
		{
			deltaY = 1;
		}
	}

	TryPickCells(deltaX, deltaY);
}

bool Enemy::TryMove()
{
	if(Map::IsSolid(targetCellX, targetCellY))
	{
		//engine.map.openDoorsAt(targetCellX, targetCellZ, Direction_None);
		return false;
	}

	int16_t targetX = (targetCellX * CELL_SIZE) + CELL_SIZE / 2;
	int16_t targetY = (targetCellY * CELL_SIZE) + CELL_SIZE / 2;

	constexpr int16_t maxDelta = 6;

	int16_t deltaX = Clamp(targetX - x, -maxDelta, maxDelta);
	int16_t deltaY = Clamp(targetY - y, -maxDelta, maxDelta);

	x += deltaX;
	y += deltaY;

	if(IsOverlappingEntity(Game::player))
	{
		x -= deltaX;
		y -= deltaY;
		return false;
	}

	if(x == targetX && y == targetY)
	{
		PickNewTargetCell();
	}
	return true;	
}

void Enemy::Tick()
{
	TryMove();
}
	
void EnemyManager::Update()
{
	for(uint8_t n = 0; n < maxEnemies; n++)
	{
		if(enemies[n].IsValid())
		{
			enemies[n].Tick();
		}
	}
}

extern const uint16_t skeletonSpriteData[];

void EnemyManager::Draw()
{
	for(uint8_t n = 0; n < maxEnemies; n++)
	{
		Enemy& enemy = enemies[n];
		if(enemy.IsValid())
		{
			Renderer::DrawObject(skeletonSpriteData, enemy.x, enemy.y);
		}
	}
}

void EnemyManager::Spawn(EnemyType enemyType, int16_t x, int16_t y)
{
	for(uint8_t n = 0; n < maxEnemies; n++)
	{
		if(!enemies[n].IsValid())
		{
			enemies[n].Init(enemyType, x, y);
			return;
		}
	}		
}

void EnemyManager::SpawnEnemies()
{
	for (uint8_t y = 0; y < Map::height; y++)
	{
		for (uint8_t x = 0; x < Map::width; x++)
		{
			switch (Map::GetCellSafe(x, y))
			{
			case CellType::Skeleton:
				EnemyManager::Spawn(EnemyType::Skeleton, x * CELL_SIZE + CELL_SIZE / 2, y * CELL_SIZE + CELL_SIZE / 2);
				Map::SetCell(x, y, CellType::Empty);
				break;
			}
		}
	}
	
}

Enemy* EnemyManager::GetOverlappingEnemy(Entity& entity)
{
	for (uint8_t n = 0; n < EnemyManager::maxEnemies; n++)
	{
		Enemy& enemy = EnemyManager::enemies[n];
		if (enemy.IsValid() && enemy.IsOverlappingEntity(entity))
		{
			return &enemy;
		}
	}

	return nullptr;
}