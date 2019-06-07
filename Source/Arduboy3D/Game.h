#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Player.h"

class Entity;

class Game
{
public:
	enum class State : uint8_t
	{
		Menu,
		InGame
	};

	static void Init();
	static void Tick();
	static void Draw();

	static void StartGame();
	static void NextLevel();
	
	static void SwitchState(State newState);

	static void ShowMessage(const char* message);

	static Player player;

	static const char* displayMessage;
	static uint8_t displayMessageTime;

private:
	static void TickCamera();
	static void TickInGame();
	
	static State state;
};
