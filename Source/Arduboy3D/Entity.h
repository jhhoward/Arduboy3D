#pragma once

#include <stdint.h>

class Entity
{
public:
	bool IsOverlappingEntity(const Entity& other) const;
	bool IsWorldColliding();

	int16_t x, y;
};
