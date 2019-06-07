#include "Defines.h"
#include "Platform.h"
#include "Menu.h"
#include "Font.h"
#include "Game.h"

int Menu::selection = 0;

void Menu::Init()
{
	selection = 0;
}

void Menu::Draw()
{
	Platform::FillScreen(COLOUR_WHITE);
	Font::PrintString(PSTR("Catacombs of the damned"), 2, 18);
	Font::PrintString(PSTR("Start"), 4, 24);
	Font::PrintString(PSTR("Sound:"), 5, 24);

	if (Platform::IsAudioEnabled())
	{
		Font::PrintString(PSTR("on"), 5, 52);
	}
	else
	{
		Font::PrintString(PSTR("off"), 5, 52);
	}
	
	Font::PrintString(PSTR(">"), 4 + selection, 16);
}

void Menu::Tick()
{
	static uint8_t lastInput = 0;

	if ((Platform::GetInput() & INPUT_DOWN) && !(lastInput & INPUT_DOWN))
	{
		selection = !selection;
	}
	if ((Platform::GetInput() & INPUT_UP) && !(lastInput & INPUT_UP))
	{
		selection = !selection;
	}

	if ((Platform::GetInput() & (INPUT_A | INPUT_B)) && !(lastInput & (INPUT_A | INPUT_B)))
	{
		switch (selection)
		{
		case 0:
			Game::StartGame();
			break;
		case 1:
			Platform::SetAudioEnabled(!Platform::IsAudioEnabled());
			break;
		}
	}

	lastInput = Platform::GetInput();
}

