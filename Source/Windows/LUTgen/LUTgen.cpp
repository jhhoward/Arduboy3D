#include <stdio.h>
#include "Defines.h"
#include "FixedMath.h"

#define SCALE_LUT_SIZE (((MAX_SPRITE_SIZE + 2) / 2) * ((MAX_SPRITE_SIZE + 2) / 2))
int16_t gen_sinTable[FIXED_ANGLE_MAX];
uint8_t gen_scaleLUT[SCALE_LUT_SIZE];

void GenerateLUT()
{
	for (int n = 0; n < FIXED_ANGLE_MAX; n++)
	{
		gen_sinTable[n] = FLOAT_TO_FIXED(sin(FIXED_ANGLE_TO_RADIANS(n)));
	}

	{
		int pos = 0;

		for (int n = 0; n <= MAX_SPRITE_SIZE; n += 2)
		{
			for (int i = 0; i < n; i++)
			{
				uint8_t u = (uint8_t)((i * BASE_SPRITE_SIZE) / n);
				gen_scaleLUT[pos++] = u;
			}
			gen_scaleLUT[pos++] = BASE_SPRITE_SIZE - 1;
		}
	}
}

int main(int argc, char* argv[])
{
	GenerateLUT();

	FILE* fs;
	fopen_s(&fs, "LUT.h", "w");

	fprintf(fs, "const uint8_t scaleLUT[] PROGMEM = {\n\t");
	for (int n = 0; n < SCALE_LUT_SIZE; n++)
	{
		fprintf(fs, "%d", gen_scaleLUT[n]);
		if (n != SCALE_LUT_SIZE - 1)
		{
			fprintf(fs, ",");
		}
	}
	fprintf(fs, "\n};\n\n");

	fprintf(fs, "const int16_t sinTable[] PROGMEM = {\n\t");
	for (int n = 0; n < FIXED_ANGLE_MAX; n++)
	{
		fprintf(fs, "%d", gen_sinTable[n]);
		if (n != FIXED_ANGLE_MAX - 1)
		{
			fprintf(fs, ",");
		}
	}
	fprintf(fs, "\n};\n\n");

	fclose(fs);

	return 0;
}