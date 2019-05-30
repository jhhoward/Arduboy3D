#include "Entity.h"
#include "Game.h"
#include "Map.h"

#define COLLISION_SIZE 48
#define ENTITY_SIZE 192

bool Entity::IsWorldColliding()
{
	return Map::IsBlockedAtWorldPosition(x - COLLISION_SIZE, y - COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x + COLLISION_SIZE, y - COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x + COLLISION_SIZE, y + COLLISION_SIZE)
		|| Map::IsBlockedAtWorldPosition(x - COLLISION_SIZE, y + COLLISION_SIZE);
}

bool Entity::IsOverlappingEntity(const Entity& other) const
{
	return (x >= other.x - ENTITY_SIZE && x <= other.x + ENTITY_SIZE
		&& y >= other.y - ENTITY_SIZE && y <= other.y + ENTITY_SIZE);
}

