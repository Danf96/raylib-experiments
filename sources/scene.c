#include "scene.h"
#include "model.h"

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

void UpdateDirtyEntities(EntityList *entityList, HeightMap *heightMap)
{
  for (size_t i = 0; i < entityList->size; i++)
  {
    if (entityList->entities[i].isDirty)
    {
      Entity *dirtyEnt = &entityList->entities[i];
      Vector3 adjustedPos =
          (Vector3){.x = dirtyEnt->position.x - terrainOffset.x,
                    .z = dirtyEnt->position.z - terrainOffset.y,
                    .y = dirtyEnt->position.y + GetAdjustedPosition(dirtyEnt->position, heightMap)};
      dirtyEnt->worldMatrix = MatrixMultiply(
          MatrixRotateXYZ(dirtyEnt->rotation),
          MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                         MatrixScale(dirtyEnt->scale.x, dirtyEnt->scale.y,
                                     dirtyEnt->scale.z)));
      dirtyEnt->isDirty = false;
    }
  }
}