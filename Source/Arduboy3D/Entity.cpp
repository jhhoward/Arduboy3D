#include "Entity.h"
#include "Game.h"
#include "Map.h"

#define COLLISION_SIZE 48

bool Entity::IsWorldColliding()
{
	return Map::IsBlockedAtWorldPosition(x - COLLISION_SIZE, y - COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x + COLLISION_SIZE, y - COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x + COLLISION_SIZE, y + COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x - COLLISION_SIZE, y + COLLISION_SIZE);
}

bool Entity::CheckCollisions(MoveResult& collisionResult)
{
	if (IsWorldColliding())
	{
		collisionResult.didCollide = true;
		return true;
	}

	return false;
}

MoveResult Entity::Move(int16_t deltaX, int16_t deltaY)
{
	MoveResult result;
	result.collidedEntity = nullptr;
	result.didCollide = false;

	x += deltaX;
	y += deltaY;
	
	if (CheckCollisions(result))
	{
		y -= deltaY;
		if (CheckCollisions(result))
		{
			x -= deltaX;
			y += deltaY;

			if (CheckCollisions(result))
			{
				y -= deltaY;
			}
		}
	}

	return result;
}
