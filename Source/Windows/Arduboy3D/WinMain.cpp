#include <SDL.h>
#include <stdio.h>
#include <sstream>
#include <iomanip>
#include <vector>
#include "Defines.h"
#include "Game.h"
#include "Map.h"
#include "Draw.h"
#include "Platform.h"
#include "FixedMath.h"
#include "MapGenerator.h"
#include "lodepng.h"

#define ZOOM_SCALE 1
#define TONES_END 0x8000

#include "Data_Audio.h"

SDL_Window* AppWindow;
SDL_Renderer* AppRenderer;
SDL_Surface* ScreenSurface;
SDL_Texture* ScreenTexture;

uint8_t InputMask = 0;
uint8_t sBuffer[DISPLAY_WIDTH * DISPLAY_HEIGHT / 8];

bool isAudioEnabled = true;
bool IsRecording = false;
int CurrentRecordingFrame = 0;

struct KeyMap
{
	SDL_Scancode key;
	uint8_t mask;
};

std::vector<KeyMap> KeyMappings =
{
	{ SDL_SCANCODE_LEFT, INPUT_LEFT },
	{ SDL_SCANCODE_RIGHT, INPUT_RIGHT },
	{ SDL_SCANCODE_UP, INPUT_UP },
	{ SDL_SCANCODE_DOWN, INPUT_DOWN },
	{ SDL_SCANCODE_Z, INPUT_A },
	{ SDL_SCANCODE_X, INPUT_B },
};

constexpr int audioSampleRate = 48000;

const uint16_t* currentAudioPattern = nullptr;
int currentPatternBufferPos = 0;

void Play(const uint16_t* pattern)
{
	currentAudioPattern = pattern;
	currentPatternBufferPos = 0;
}

void FillAudioBuffer(void *udata, uint8_t *stream, int len)
{
	int feedPos = 0;
	
	static int waveSamplesLeft = 0;
	static int noteSamplesLeft = 0;
	static int frequency = 0;
	static bool high = false;

	while(feedPos < len)
	{
		if(!isAudioEnabled)
		{
			while(feedPos < len)
			{
				stream[feedPos++] = 0;
			}
			return;
		}
		
		if(currentAudioPattern != nullptr)
		{
			if(noteSamplesLeft == 0)
			{
				frequency = currentAudioPattern[currentPatternBufferPos];
				uint16_t duration = currentAudioPattern[currentPatternBufferPos + 1];
				
				noteSamplesLeft = (audioSampleRate * duration) / 1000;
				
				waveSamplesLeft = frequency > 0 ? audioSampleRate / frequency : noteSamplesLeft;
				
				currentPatternBufferPos += 2;
				if(currentAudioPattern[currentPatternBufferPos] == TONES_END)
				{
					currentAudioPattern = nullptr;
				}
			}
		}
		
		if(frequency == 0)
		{
			while(feedPos < len && (!currentAudioPattern || noteSamplesLeft > 0))
			{
				stream[feedPos++] = 0;
				
				if(noteSamplesLeft > 0)
					noteSamplesLeft--;
			}
		}
		else
		{
			while(feedPos < len && waveSamplesLeft > 0 && noteSamplesLeft > 0)
			{
				int volume = 32;
				stream[feedPos++] = high ? 128 + volume : 128 - volume;
				waveSamplesLeft--;
				noteSamplesLeft--;
			}
			
			if(waveSamplesLeft == 0)
			{
				high = !high;
				waveSamplesLeft = audioSampleRate / frequency;
			}
		}
		
	}
}

void Platform::SetLED(uint8_t r, uint8_t g, uint8_t b)
{

}

void Platform::FillScreen(uint8_t colour)
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			PutPixel(x, y, colour);
		}
	}
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, uint8_t frame)
{
	uint8_t w = bitmap[0];
	uint8_t h = bitmap[1];

	bitmap += 2;

	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockY = j / 8;
			int blockIndex = (w * blockY + i) * 2;
			uint8_t pixels = bitmap[blockIndex];
			uint8_t maskPixels = bitmap[blockIndex + 1];
			uint8_t bitmask = 1 << (j % 8);

			if (maskPixels & bitmask)
			{
				if (x + i >= 0 && y + j >= 0)
				{
					if (pixels & bitmask)
					{
						PutPixel(x + i, y + j, 1);
					}
					else
					{
						PutPixel(x + i, y + j, 0);
					}
				}
			}
		}
	}
}

void Platform::DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap, const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
	uint8_t w = bitmap[0];
	uint8_t h = bitmap[1];

	bitmap += 2;

	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockY = j / 8;
			int blockIndex = w * blockY + i;
			uint8_t pixels = bitmap[blockIndex];
			uint8_t maskPixels = mask[blockIndex];
			uint8_t bitmask = 1 << (j % 8);

			if (maskPixels & bitmask)
			{
				if (x + i >= 0 && y + j >= 0)
				{
					if (pixels & bitmask)
					{
						PutPixel(x + i, y + j, 1);
					}
					else
					{
						PutPixel(x + i, y + j, 0);
					}
				}
			}
		}
	}
}

void Platform::DrawVLine(uint8_t x, int8_t y1, int8_t y2, uint8_t pattern)
{
	for (int y = y1; y <= y2; y++)
	{
		if (y >= 0)
		{
			uint8_t patternIndex = y % 8;
			uint8_t mask = 1 << patternIndex;
			PutPixel(x, y, (mask & pattern) != 0 ? 1 : 0);
		}
	}
}

/*
// Adpated from https://github.com/a1k0n/arduboy3d/blob/master/draw.cpp
// since the AVR has no barrel shifter, we'll do a progmem lookup
const uint8_t topmask_[] PROGMEM = {
	0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };
const uint8_t bottommask_[] PROGMEM = {
	0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

void DrawVLine(uint8_t x, int8_t y0_, int8_t y1_, uint8_t pattern) 
{
	uint8_t *screenptr = arduboy.getBuffer() + x;

	if (y1_ < y0_ || y1_ < 0 || y0_ > 63) return;

	// clip (FIXME; clipping should be handled elsewhere)
	// cast to unsigned after clipping to simplify generated code below
	uint8_t y0 = y0_, y1 = y1_;
	if (y0_ < 0) y0 = 0;
	if (y1_ > 63) y1 = 63;

	uint8_t *page0 = screenptr + ((y0 & 0x38) << 4);
	uint8_t *page1 = screenptr + ((y1 & 0x38) << 4);
	if (page0 == page1) 
	{
		uint8_t mask = pgm_read_byte(topmask_ + (y0 & 7))
			& pgm_read_byte(bottommask_ + (y1 & 7));
		*page0 &= ~mask;
		*page0 |= pattern & mask;  // fill y0..y1 in same page in one shot
	}
	else
	{
		uint8_t mask = pgm_read_byte(topmask_ + (y0 & 7));
		*page0 &= ~mask;
		*page0 |= pattern & mask;  // write top 1..8 pixels
		page0 += 128;
		while (page0 != page1) 
		{
			*page0 = pattern;  // fill middle 8 pixels at a time
			page0 += 128;
		}
		mask = pgm_read_byte(bottommask_ + (y1 & 7));  // and bottom 1..8 pixels
		*page0 &= ~mask;
		*page0 |= pattern & mask;
	}
}
*/

void Platform::PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
	{
		return;
	}

	uint16_t row_offset;
	uint8_t bit;

	bit = 1 << (y & 7);
	row_offset = (y & 0xF8) * DISPLAY_WIDTH / 8 + x;
	uint8_t data = sBuffer[row_offset] | bit;
	if (!colour) data ^= bit;
	sBuffer[row_offset] = data;
}

uint8_t GetPixel(uint8_t x, uint8_t y)
{
	uint8_t row = y / 8;
	uint8_t bit_position = y % 8;
	return (sBuffer[(row*DISPLAY_WIDTH) + x] & (1 << bit_position)) >> bit_position;
}

uint8_t* Platform::GetScreenBuffer()
{
	return sBuffer;
}

void ResolveScreen(SDL_Surface* surface)
{
	Uint32 black = SDL_MapRGBA(surface->format, 0, 0, 0, 255); 
	Uint32 white = SDL_MapRGBA(surface->format, 255, 255, 255, 255);

	int bpp = surface->format->BytesPerPixel;
	
	for(int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for(int x = 0; x < DISPLAY_WIDTH; x++)
		{
			Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
			
			*(Uint32 *)p = GetPixel(x, y) ? white : black;
		}
	}
}

void PutPixelImmediate(uint8_t x, uint8_t y, uint8_t colour)
{
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
	{
		return;
	}

	SDL_Surface* surface = ScreenSurface;

	Uint32 col = colour ? SDL_MapRGBA(surface->format, 255, 255, 255, 255) : SDL_MapRGBA(surface->format, 0, 0, 0, 255);

	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	*(Uint32 *)p = col;
}

void DrawBitmapInternal(const uint8_t* data, uint16_t x, uint16_t y, uint8_t w, uint8_t h)
{
	for (int j = 0; j < h; j++)
	{
		for (int i = 0; i < w; i++)
		{
			int blockX = i / 8;
			int blockY = j / 8;
			int blocksPerWidth = w / 8;
			int blockIndex = blockY * blocksPerWidth + blockX;
			uint8_t pixels = data[blockIndex * 8 + i % 8];
			uint8_t mask = 1 << (j % 8);
			if (x + i >= 0 && y + j >= 0)
			{
				if (pixels & mask)
				{
					Platform::PutPixel(x + i, y + j, 1);
				}
				else
				{
					Platform::PutPixel(x + i, y + j, 0);
				}
			}
		}
	}
}

void Platform::DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
	DrawBitmapInternal(bitmap + 2, x, y, bitmap[0], bitmap[1]);
}

void Platform::DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
	DrawBitmapInternal(bitmap + 2, x, y, bitmap[0], bitmap[1]);
}

void Platform::DrawBackground()
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++)
	{
		for (int x = 0; x < DISPLAY_WIDTH; x++)
		{
			uint8_t col = y < DISPLAY_HEIGHT / 2 ? (x | y) & 1 ? COLOUR_BLACK : COLOUR_WHITE : (x ^ y) & 1 ? COLOUR_BLACK : COLOUR_WHITE; //192;
																																		  //col = 192;
			PutPixel(x, y, col);
		}
	}
}

void Platform::PlaySound(const uint16_t* audioPattern)
{
	Play(audioPattern);
}

bool Platform::IsAudioEnabled()
{
	return isAudioEnabled;
}

void Platform::SetAudioEnabled(bool isEnabled)
{
	isAudioEnabled = isEnabled;
}

uint8_t Platform::GetInput()
{
	uint8_t inputMask = 0;

	const uint8_t* keyStates = SDL_GetKeyboardState(NULL);

	for (unsigned int n = 0; n < KeyMappings.size(); n++)
	{
		if (keyStates[KeyMappings[n].key])
		{
			inputMask |= KeyMappings[n].mask;
		}
	}

	return inputMask;
}

void Platform::ExpectLoadDelay()
{

}

/*#include "Font.h"
void DumpFont()
{
	constexpr int numChars = 96;
	constexpr int charWidth = 4;
	constexpr int charHeight = 8;
	constexpr int pageWidth = numChars * charWidth;
	uint8_t fontPage[pageWidth * charHeight * 4];
	char tempStr[2] = { 0, 0 };

	for (int n = 0; n < numChars * charWidth * charHeight * 4; n++)
	{
		fontPage[n] = 255;
	}

	for (int n = 0; n < numChars; n++)
	{
		Platform::FillScreen(COLOUR_WHITE);
		tempStr[0] = (char) (n + 32);
		DrawString(tempStr, 0, 1);

		for (int y = 0; y < charHeight; y++)
		{
			for (int x = 0; x < charWidth; x++)
			{
				if (GetPixel(x, y) == COLOUR_BLACK)
				{
					int index = pageWidth * y + (n * charWidth) + x;
					fontPage[index * 4] = 0;
					fontPage[index * 4 + 1] = 0;
					fontPage[index * 4 + 2] = 0;
				}
			}
		}
	}

	lodepng::encode("font.png", fontPage, pageWidth, charHeight);
}*/

void DebugDisplayNow()
{
	ResolveScreen(ScreenSurface);
	SDL_UpdateTexture(ScreenTexture, NULL, ScreenSurface->pixels, ScreenSurface->pitch);
	SDL_Rect src, dest;
	src.x = src.y = dest.x = dest.y = 0;
	src.w = DISPLAY_WIDTH;
	src.h = DISPLAY_HEIGHT;
	dest.w = DISPLAY_WIDTH;
	dest.h = DISPLAY_HEIGHT;
	SDL_RenderCopy(AppRenderer, ScreenTexture, &src, &dest);
	SDL_RenderPresent(AppRenderer);

//	SDL_Delay(1000 / TARGET_FRAMERATE);
	//SDL_Delay(1);
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_CreateWindowAndRenderer(DISPLAY_WIDTH * ZOOM_SCALE, DISPLAY_HEIGHT * ZOOM_SCALE, SDL_WINDOW_RESIZABLE, &AppWindow, &AppRenderer);
	SDL_RenderSetLogicalSize(AppRenderer, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	ScreenSurface = SDL_CreateRGBSurface(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 32,
		0x000000ff,
		0x0000ff00,
		0x00ff0000,
		0xff000000
	);
	ScreenTexture = SDL_CreateTexture(AppRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ScreenSurface->w, ScreenSurface->h);

	SDL_SetWindowPosition(AppWindow, 1900 - DISPLAY_WIDTH * 2, 1020 - DISPLAY_HEIGHT);

	SDL_AudioSpec wanted;
	wanted.freq = audioSampleRate;
	wanted.format = AUDIO_U8;
	wanted.channels = 1;
	wanted.samples = 4096;
	wanted.callback = FillAudioBuffer;

	if (SDL_OpenAudio(&wanted, NULL) <0) {
		printf("Error: %s\n", SDL_GetError());
	}
	SDL_PauseAudio(0);
	
	//DumpFont();

	//SeedRandom((uint16_t)time(nullptr));
	SeedRandom(0);
	Game::Init();
	
	bool running = true;
	int playRate = 1;
	static int testAudio = 0;

	while (running)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					running = false;
					break;
				case SDLK_i:
					testAudio--;
					if(testAudio < 0)
						testAudio = NUM_AUDIO_PATTERNS - 1;
					Play(Data_AudioPatterns[testAudio]);
					break;
				case SDLK_o:
					testAudio++;
					if(testAudio >= NUM_AUDIO_PATTERNS)
						testAudio = 0;
					Play(Data_AudioPatterns[testAudio]);
					break;
				case SDLK_p:
					Play(Data_AudioPatterns[testAudio]);
					break;
				case SDLK_TAB:
					playRate = 10;
					MapGenerator::Generate();
					break;
				case SDLK_F12:
					{
						lodepng::encode(std::string("screenshot.png"), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
					}
					break;
				case SDLK_F11:
					IsRecording = !IsRecording;
					break;
				}
				break;
			case SDL_KEYUP:
				if (event.key.keysym.sym == SDLK_TAB)
					playRate = 1;
				break;
			}
		}

		SDL_SetRenderDrawColor(AppRenderer, 206, 221, 231, 255);
		SDL_RenderClear(AppRenderer);

		for (int n = 0; n < playRate; n++)
		{
			memset(ScreenSurface->pixels, 0, ScreenSurface->format->BytesPerPixel * ScreenSurface->w * ScreenSurface->h);
			
			Game::Tick();
			Game::Draw();
			//Map::DebugDraw();
			
			ResolveScreen(ScreenSurface);
		}

		if (IsRecording)
		{
			std::ostringstream filename;
			filename << "Frame";
			filename << std::setfill('0') << std::setw(5) << CurrentRecordingFrame << ".png";

			lodepng::encode(filename.str(), (unsigned char*)(ScreenSurface->pixels), ScreenSurface->w, ScreenSurface->h);
			CurrentRecordingFrame++;
		}

		SDL_UpdateTexture(ScreenTexture, NULL, ScreenSurface->pixels, ScreenSurface->pitch);
		SDL_Rect src, dest;
		src.x = src.y = dest.x = dest.y = 0;
		src.w = DISPLAY_WIDTH;
		src.h = DISPLAY_HEIGHT;
		dest.w = DISPLAY_WIDTH;
		dest.h = DISPLAY_HEIGHT;
		SDL_RenderCopy(AppRenderer, ScreenTexture, &src, &dest);
		SDL_RenderPresent(AppRenderer);

		SDL_Delay(1000 / TARGET_FRAMERATE);
	}

	return 0;
}
