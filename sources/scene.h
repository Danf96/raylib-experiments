#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "terrain.h"
#include "camera.h"

#define GAME_MAX_UNITS 100
#define GAME_MAX_SELECTED 12

typedef enum
{
  ENT_TYPE_EMPTY = 0,
  ENT_TYPE_ACTOR,
} EntityType;

typedef enum
{
  ENT_IDLE = 0,
  ENT_IS_MOVING = (1<<1),
  ENT_IS_ATTACKING = (1<<2),
  ENT_DEAD = (1<<3),
} EntityState;

typedef struct
{
  short id;
  float offsetY;
  Vector3 scale;
  Vector3 position;
  Vector3 dimensions;
  Vector3 rotation;
  Vector2 targetPos;
  float moveSpeed;
  Matrix worldMatrix;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
  BoundingBox bbox;
  EntityState state;
  bool isDirty;
} Entity;

typedef struct
{
  Vector3 scale;
  Vector2 position;
  Vector3 dimensions;
  Vector3 rotation;
  float offsetY;
  float moveSpeed;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
} EntityCreate;

typedef struct
{
  size_t capacity;
  size_t size;
  Entity *entities;
  short selected[GAME_MAX_SELECTED];
} EntityList;


extern Vector2 terrainOffset;

int AddEntity(EntityList *entityList, EntityCreate *entityCreate);

EntityList CreateEntityList(size_t capacity);

void UpdateEntities(EntityList *entityList, TerrainMap *terrainMap, RTSCamera *camera);

BoundingBox EntityBBoxDerive(Vector3 *position, Vector3 *dimensions, Vector3 *scale);

void EntitySetMoving(Vector2 position, short entityId, EntityList *entityList);

void EntitySelectedAdd(short selectedId, EntityList *entityList);

void EntitySelectedRemoveAll(EntityList *entityList);

void EntitySelectedRemove(short selectedId, EntityList *entityList);

short EntityGetSelectedId(Ray ray, EntityList *entityList);

void EntityUpdateDirty(Entity *ent, TerrainMap *terrainMap);

void EntityCheckCollision(Entity *ent, EntityList *entityList);