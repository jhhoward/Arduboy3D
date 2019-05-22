#pragma once

#include <stdint.h>
#include "Defines.h"
#include "Player.h"

class Entity;

class Game
{
public:
	static void Init();
	static void Tick();

	static Player player;

private:
	static void TickCamera();
};
