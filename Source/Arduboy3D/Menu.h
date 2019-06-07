#pragma once

#include <stdint.h>

class Menu
{
public:
	static void Init();
	static void Draw();
	static void Tick();
	
private:
	static int selection;
};
