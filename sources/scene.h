#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

#include "raylib.h"
#include "raymath.h"

#include "model.h"



typedef enum {
  NODE_TYPE_EMPTY,
  NODE_TYPE_MODEL,
  NODE_TYPE_BILLBOARD,
  NODE_TYPE_LIGHT,
} EntityType;

typedef struct {
  short id;
  Vector3 scale;
  Vector3 position;
  Vector3 velocity;
  Vector3 rotation;
  Matrix worldMatrix;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
  bool isDirty;
} Entity;

typedef struct {
  Vector3 scale;
  Vector3 position;
  Vector3 rotation;
  short int typeHandle;
  short int materialHandle;
  EntityType type;
} EntityCreate;

typedef struct {
  size_t capacity;
  size_t size;
  Entity *entities;
} EntityList;


extern Vector2 terrainOffset;

int AddEntity(EntityList *entityList, EntityCreate *entityCreate);

EntityList CreateEntityList(size_t capacity);
void UpdateDirtyEntities(EntityList *entityList, HeightMap *heightMap);

