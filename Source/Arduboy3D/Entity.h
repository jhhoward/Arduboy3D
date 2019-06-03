#pragma once

#include <stdint.h>

class Entity
{
public:
	bool IsOverlappingPoint(int16_t pointX, int16_t pointY) const;
	bool IsOverlappingEntity(const Entity& other) const;
	bool IsWorldColliding() const;

	int16_t x, y;
};
