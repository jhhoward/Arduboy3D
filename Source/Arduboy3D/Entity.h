#pragma once

#include <stdint.h>

struct MoveResult
{
	bool didCollide;
	class Entity* collidedEntity;
};

class Entity
{
public:
	MoveResult Move(int16_t deltaX, int16_t deltaY);
	
	bool CheckCollisions(MoveResult& collisionResult);
	bool IsWorldColliding();
	

	int16_t x, y;
};
