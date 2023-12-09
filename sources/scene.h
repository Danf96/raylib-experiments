#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#include "terrain.h"
#include "camera.h"


typedef enum
{
  ENT_TYPE_EMPTY = 0,
  ENT_TYPE_ACTOR,
} EntityType;

typedef struct
{
  short id;
  Vector3 scale;
  Vector3 position;
  Vector3 dimensions;
  Vector3 velocity;
  Vector3 rotation;
  Vector2 targetPos;
  float moveSpeed;
  Matrix worldMatrix;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
  BoundingBox bbox;
  bool isMoving;
  bool isDirty;
} Entity;

typedef struct
{
  Vector3 scale;
  Vector3 position;
  Vector3 dimensions;
  Vector3 rotation;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
} EntityCreate;

typedef struct
{
  size_t capacity;
  size_t size;
  Entity *entities;
} EntityList;

extern Vector2 terrainOffset;

int AddEntity(EntityList *entityList, EntityCreate *entityCreate);

EntityList CreateEntityList(size_t capacity);
void UpdateEntities(EntityList *entityList, TerrainMap *terrainMap, RTSCamera *camera);

BoundingBox DeriveBBox(Vector3 *position, Vector3 *dimensions, Vector3 *scale);

void MoveEntity(Vector2 position, short entityId, EntityList *entityList);

