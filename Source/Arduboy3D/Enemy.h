#pragma once

#include <stdint.h>
#include "Entity.h"

enum class EnemyType : uint8_t
{
	None,
	Skeleton
};

class Enemy : public Entity
{
public:
	void Init(EnemyType type, int16_t x, int16_t y);
	void Tick();
	bool IsValid() const { return type != EnemyType::None; }
	void Damage() { type = EnemyType::None; }
	
private:
	bool TryMove();
	void PickNewTargetCell();
	bool TryPickCells(int8_t deltaX, int8_t deltaY);
	bool TryPickCell(int8_t newX, int8_t newY);

	EnemyType type;
	uint8_t targetCellX, targetCellY;
};

class EnemyManager
{
public:
	static constexpr int maxEnemies = 8;
	static Enemy enemies[maxEnemies];
	
	static void Spawn(EnemyType enemyType, int16_t x, int16_t y);
	static void SpawnEnemies();

	static Enemy* GetOverlappingEnemy(Entity& entity);
	
	static void Draw();
	static void Update();
};
