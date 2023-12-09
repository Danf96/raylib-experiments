#include "scene.h"

static size_t GLOBAL_ID = 0;
static int selectedId = -1;

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
                           .isMoving = false,
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

// eventually pull out of scene
void UpdateEntities(EntityList *entityList, TerrainMap *terrainMap, RTSCamera *camera)
{
  Ray mRay = {};
  if (camera->mouseButton == MOUSE_BUTTON_LEFT || camera->mouseButton == MOUSE_BUTTON_RIGHT)
  {
    mRay = GetMouseRay(GetMousePosition(), camera->ViewCamera);
  }
  if (camera->mouseButton == MOUSE_BUTTON_LEFT)
  {
    RayCollision collision = {};
    for (size_t i = 0; i < entityList->size; i++)
    {
      collision = GetRayCollisionBox(mRay, entityList->entities[i].bbox);
      if (collision.hit)
      {
        selectedId = i;
        break;
      }
    }
    if (!collision.hit)
    {
      selectedId = -1;
    }
  }
  else if (camera->mouseButton == MOUSE_BUTTON_RIGHT)
    {
      if (selectedId >= 0)
      {
        Vector3 target = GetRayPointTerrain(mRay, terrainMap, camera->NearPlane, camera->FarPlane);
        MoveEntity((Vector2){target.x, target.z}, selectedId, entityList);
      }
    }
  for (size_t i = 0; i < entityList->size; i++)
  {
    // update entities here then mark dirty
    Entity *Ent = &entityList->entities[i];
    if (Ent->isMoving)
    {
      if (Vector2Equals((Vector2){Ent->position.x, Ent->position.z}, Ent->targetPos)){
        Ent->isMoving = false;
      }
      float adjustedSpeed = Ent->moveSpeed * GetFrameTime(); //pass to update loop later
      if (Ent->isMoving)
      {
        Vector2 moveVec = Vector2Scale(Vector2Normalize(Vector2Subtract((Vector2){Ent->position.x, Ent->position.z}, Ent->targetPos)), adjustedSpeed);
        Ent->position = Vector3Add((Vector3){moveVec.x, 0.0, moveVec.y}, Ent->position);
      }
    }
    
    if (Ent->isDirty)
    {
      Vector3 adjustedPos = (Vector3){.x = Ent->position.x,
                                      .z = Ent->position.z,
                                      .y = Ent->position.y + GetAdjustedHeight(Ent->position, terrainMap)};
      Ent->worldMatrix = MatrixMultiply(MatrixRotateXYZ(Ent->rotation),
                                             MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                                                            MatrixScale(Ent->scale.x, Ent->scale.y, Ent->scale.z)));
      Ent->bbox = DeriveBBox(&adjustedPos, &Ent->dimensions, &Ent->scale);
      Ent->isDirty = false;
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

void MoveEntity(Vector2 position, short entityId, EntityList *entityList)
{
  Entity *targetEntity = &entityList->entities[entityId];
  targetEntity->targetPos = position;
  targetEntity->isMoving = true;
}