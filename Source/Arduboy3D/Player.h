#pragma once

#include <stdint.h>
#include "Entity.h"

class Player : public Entity
{
public:
	uint8_t angle;
	int16_t velocityX, velocityY;
	int8_t angularVelocity;

	uint8_t shakeTime;
	uint8_t reloadTime;

	void Tick();
	void Fire();
};
