#include <Arduboy2.h>
#include "Game.h"

Arduboy2Base arduboy;
Sprites sprites;

uint8_t GetInput()
{
  uint8_t result = 0;
  
  if(arduboy.pressed(A_BUTTON))
  {
    result |= INPUT_A;  
  }
  if(arduboy.pressed(B_BUTTON))
  {
    result |= INPUT_B;  
  }
  if(arduboy.pressed(UP_BUTTON))
  {
    result |= INPUT_UP;  
  }
  if(arduboy.pressed(DOWN_BUTTON))
  {
    result |= INPUT_DOWN;  
  }
  if(arduboy.pressed(LEFT_BUTTON))
  {
    result |= INPUT_LEFT;  
  }
  if(arduboy.pressed(RIGHT_BUTTON))
  {
    result |= INPUT_RIGHT;  
  }

  return result;
}

void SetLED(uint8_t r, uint8_t g, uint8_t b)
{
  arduboy.digitalWriteRGB(r ? RGB_ON : RGB_OFF, g ? RGB_ON : RGB_OFF, b ? RGB_ON : RGB_OFF);
}

void PutPixel(uint8_t x, uint8_t y, uint8_t colour)
{
  arduboy.drawPixel(x, y, colour);
}

void DrawSprite(int16_t x, int16_t y, const uint8_t *bitmap,
  const uint8_t *mask, uint8_t frame, uint8_t mask_frame)
{
  sprites.drawExternalMask(x, y, bitmap, mask, frame, mask_frame);
}

void DrawBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
  uint8_t w = pgm_read_byte(&bitmap[0]);
  uint8_t h = pgm_read_byte(&bitmap[1]);
  arduboy.drawBitmap(x, y, bitmap + 2, w, h);
}

void DrawSolidBitmap(int16_t x, int16_t y, const uint8_t *bitmap)
{
  uint8_t w = pgm_read_byte(&bitmap[0]);
  uint8_t h = pgm_read_byte(&bitmap[1]);
  arduboy.fillRect(x, y, w, h, BLACK);
  arduboy.drawBitmap(x, y, bitmap + 2, w, h);
}

void FillScreen(uint8_t colour)
{
  arduboy.fillScreen(colour);
}

/*
void DrawFilledRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour)
{
  arduboy.fillRect(x, y, w, h, colour);
}

void DrawRect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t colour)
{
  arduboy.drawRect(x, y, w, h, colour);
}
*/
/*
void DrawBitmap(const uint8_t* bmp, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
  arduboy.drawBitmap(x, y, bmp, w, h, WHITE);
}*/

void setup()
{
  arduboy.boot();
  arduboy.flashlight();
  arduboy.systemButtons();
  arduboy.bootLogo();
  arduboy.setFrameRate(30);

  //Serial.begin(9600);

  InitGame();
}

void loop()
{
  if(arduboy.nextFrame())
  {
    TickGame();
    
    //Serial.write(arduboy.getBuffer(), 128 * 64 / 8);
    arduboy.display(true);
  }
}

