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
	EnemyManager::Init();
	MapGenerator::Generate();

	player.x = CELL_SIZE * 1 + CELL_SIZE / 2;
	player.y = CELL_SIZE * 1 + CELL_SIZE / 2;
	
	EnemyManager::SpawnEnemies();
	//EnemyManager::Spawn(EnemyType::Skeleton, player.x + CELL_SIZE * 3, player.y);
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
}