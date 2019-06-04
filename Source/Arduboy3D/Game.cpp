#include "Defines.h"
#include "Game.h"
#include "FixedMath.h"
#include "Draw.h"
#include "Map.h"
#include "Projectile.h"
#include "Particle.h"
#include "MapGenerator.h"
#include "Platform.h"
#include "Entity.h"
#include "Enemy.h"

Player Game::player;

void Game::Init()
{
	NextLevel();
}

void Game::NextLevel()
{
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();
	MapGenerator::Generate();
	EnemyManager::SpawnEnemies();

	player.Init();
}

void Game::Tick()
{
	Renderer::globalAnimationFrame++;

	player.Tick();

	Renderer::camera.x = player.x;
	Renderer::camera.y = player.y;
	Renderer::camera.angle = player.angle;

	ProjectileManager::Update();
	ParticleSystemManager::Update();
	EnemyManager::Update();

	if (Map::GetCellSafe(player.x / CELL_SIZE, player.y / CELL_SIZE) == CellType::Exit)
	{
		NextLevel();
	}

	if (player.hp == 0)
		NextLevel();
}