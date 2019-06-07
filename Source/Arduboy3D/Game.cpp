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
#include "Menu.h"

Player Game::player;
const char* Game::displayMessage = nullptr;
uint8_t Game::displayMessageTime = 0;
Game::State Game::state = Game::State::Menu;

void Game::Init()
{
	Menu::Init();
	NextLevel();
}

void Game::StartGame()
{
	NextLevel();
	state = State::InGame;
}

void Game::SwitchState(State newState)
{
	if(state != newState)
	{
		state = newState;
	}
}

void Game::ShowMessage(const char* message)
{
	constexpr uint8_t messageDisplayTime = 90;

	displayMessage = message;
	displayMessageTime = messageDisplayTime;
}

void Game::NextLevel()
{
	ParticleSystemManager::Init();
	ProjectileManager::Init();
	EnemyManager::Init();
	MapGenerator::Generate();
	EnemyManager::SpawnEnemies();

	player.Init();

	Platform::ExpectLoadDelay();
}

void Game::Draw()
{
	switch(state)
	{
		case State::InGame:
			Renderer::Render();
			break;
		case State::Menu:
			Menu::Draw();
			break;
	}
}

void Game::TickInGame()
{
	if (displayMessageTime > 0)
	{
		displayMessageTime--;
		if (displayMessageTime == 0)
			displayMessage = nullptr;
	}

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

void Game::Tick()
{
	switch(state)
	{
		case State::InGame:
			TickInGame();
			return;
		case State::Menu:
			Menu::Tick();
			return;
	}
}
