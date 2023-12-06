#include "scene.h"

static size_t GLOBAL_ID = 0;

int AddEntity(EntityList *entityList, EntityCreate *entityCreate)
{
  if (entityList->capacity <= entityList->size + 1)
  {
    // regrow array
    Entity *newArr;
    newArr = MemRealloc(entityList->entities, entityList->capacity * 2);
    if (newArr)
    {
      entityList->entities = newArr;
      entityList->capacity *= 2;
    }
    else
    {
      TraceLog(LOG_ERROR,
               "Failed to reallocate entity memory, closing program.");
      return 1;
    }
  }
  Entity entity = (Entity){.position = entityCreate->position,
                           .rotation = entityCreate->rotation,
                           .dimensions = entityCreate->dimensions,
                           .scale = entityCreate->scale,
                           .type = entityCreate->type,
                           .typeHandle = entityCreate->typeHandle,
                           .materialHandle = entityCreate->materialHandle,
                           .id = GLOBAL_ID,
                           .isDirty = true};
  entityList->entities[entityList->size] = entity;
  entityList->size++;
  GLOBAL_ID++;
  return 0;
}

EntityList CreateEntityList(size_t capacity)
{
  EntityList newList = {.capacity = capacity};
  newList.entities = MemAlloc(sizeof(*newList.entities) * capacity);
  return newList;
}

void UpdateDirtyEntities(EntityList *entityList, TerrainMap *terrainMap)
{
  for (size_t i = 0; i < entityList->size; i++)
  {
    if (entityList->entities[i].isDirty)
    {
      Entity *dirtyEnt = &entityList->entities[i];
      Vector3 adjustedPos = (Vector3){.x = dirtyEnt->position.x,
                                      .z = dirtyEnt->position.z,
                                      .y = dirtyEnt->position.y + GetAdjustedHeight(dirtyEnt->position, terrainMap)};
      dirtyEnt->worldMatrix = MatrixMultiply(MatrixRotateXYZ(dirtyEnt->rotation),
                                             MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                                                            MatrixScale(dirtyEnt->scale.x, dirtyEnt->scale.y, dirtyEnt->scale.z)));
      dirtyEnt->bbox = DeriveBBox(&adjustedPos, &dirtyEnt->dimensions, &dirtyEnt->scale);
      dirtyEnt->isDirty = false;
    }
  }
}

BoundingBox DeriveBBox(Vector3 *position, Vector3 *dimensions, Vector3 *scale)
{
  return (BoundingBox){
      (Vector3){position->x - (dimensions->x * scale->x) / 2.0f,
                position->y - (dimensions->y * scale->y) / 2.0f,
                position->z - (dimensions->z * scale->z) / 2.0f},
      (Vector3){position->x + (dimensions->x * scale->x) / 2.0f,
                position->y + (dimensions->y * scale->y) / 2.0f,
                position->z + (dimensions->z * scale->z) / 2.0f},
  };
}