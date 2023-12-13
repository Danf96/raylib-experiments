#include "scene.h"
#include <string.h>

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
  Entity entity = (Entity){.position = (Vector3){entityCreate->position.x, entityCreate->offsetY, entityCreate->position.y},
                           .offsetY = entityCreate->offsetY,
                           .rotation = entityCreate->rotation,
                           .dimensions = entityCreate->dimensions,
                           .scale = entityCreate->scale,
                           .type = entityCreate->type,
                           .typeHandle = entityCreate->typeHandle,
                           .materialHandle = entityCreate->materialHandle,
                           .id = GLOBAL_ID,
                           .moveSpeed = entityCreate->moveSpeed,
                           .isDirty = true};
  entityList->entities[entityList->size] = entity;
  entityList->size++;
  GLOBAL_ID++;
  return 0;
}

EntityList CreateEntityList(size_t capacity)
{
  EntityList newList = {.capacity = capacity};
  memset(newList.selected, -1, sizeof(newList.selected));
  newList.entities = MemAlloc(sizeof(*newList.entities) * capacity);
  return newList;
}

void UpdateEntities(EntityList *entityList, TerrainMap *terrainMap, RTSCamera *camera)
{
  // iterate over entitylist here for attacking entities, check attack ranges
  if (camera->Focused)
  {
    Ray ray = GetMouseRay(GetMousePosition(), camera->ViewCamera); // we should be always getting mouse data (can change to attack icon later when mousing over enemy)
    if (camera->IsButtonPressed && camera->ClickTimer <= 0)
    {
      if (camera->MouseButton == MOUSE_BUTTON_LEFT)
      {
        short id = EntityGetSelectedId(ray, entityList);
        if (id >= 0)
        {
          if (camera->ModifierKey & ADDITIONAL_MODIFIER)
          {
            EntitySelectedAdd(id, entityList);
          }
          else
          {
            EntitySelectedRemoveAll(entityList);
            EntitySelectedAdd(id, entityList);
          }
        }
        else
        {
          EntitySelectedRemoveAll(entityList);
        }
      }
      else if (camera->MouseButton == MOUSE_BUTTON_RIGHT)
      {

        if (camera->ModifierKey & ATTACK_MODIFIER)
        {
          // TODO: write function for attacking, maybe getting the entity ID of the target so that target positions can be updated as one entity moves over
        }
        else
        {
          Vector3 target = GetRayPointTerrain(ray, terrainMap, camera->NearPlane, camera->FarPlane);
          for (int i = 0; i < GAME_MAX_SELECTED; i++)
          {
            if (entityList->selected[i] >= 0)
            {
              MoveEntity((Vector2){target.x, target.z}, entityList->selected[i], entityList);
            }
          }
        }
      }
      camera->ClickTimer = 0.2f; // add delay to input
    }
  }
  for (size_t i = 0; i < entityList->size; i++)
  {
    // update entities here then mark dirty
    Entity *ent = &entityList->entities[i];
    if (ent->state & ENT_IS_MOVING)
    {
      if (Vector2Equals((Vector2){ent->position.x, ent->position.z}, ent->targetPos))
      {
        ent->state ^= ENT_IS_MOVING;
      }
      else
      {
        float adjustedSpeed = ent->moveSpeed;
        Vector2 rawDist = Vector2Subtract(ent->targetPos, (Vector2){ent->position.x, ent->position.z});
        Vector2 moveVec = Vector2Scale(Vector2Normalize(rawDist), adjustedSpeed);
        if (Vector2Length(rawDist) < Vector2Length(moveVec))
        {
          moveVec = rawDist;
        }
#if 0
        TraceLog(LOG_INFO, TextFormat("x: %f z: %f", moveVec.x, moveVec.y));
#endif
        ent->position = Vector3Add((Vector3){moveVec.x, 0.0, moveVec.y}, ent->position);
      }
      ent->isDirty = true;
    }

    if (ent->isDirty)
    {
      Vector3 adjustedPos = (Vector3){.x = ent->position.x,
                                      .z = ent->position.z,
                                      .y = ent->offsetY + GetAdjustedHeight(ent->position, terrainMap)};
      ent->position = adjustedPos;
      ent->worldMatrix = MatrixMultiply(MatrixRotateXYZ(ent->rotation),
                                        MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                                                       MatrixScale(ent->scale.x, ent->scale.y, ent->scale.z)));
      ent->bbox = DeriveBBox(&adjustedPos, &ent->dimensions, &ent->scale);
      ent->isDirty = false;
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
  // used purely for move orders, negates attack
  Entity *targetEntity = &entityList->entities[entityId];
  targetEntity->targetPos = position;
  targetEntity->state |= ENT_IS_MOVING;
}

short EntityGetSelectedId(Ray ray, EntityList *entityList)
{
  float closestHit = __FLT_MAX__;
  short selectedId = -1;
  for (size_t i = 0; i < entityList->size; i++)
  {
    RayCollision collision = GetRayCollisionBox(ray, entityList->entities[i].bbox);
    if (collision.hit)
    {
      if (collision.distance < closestHit)
      {
        selectedId = i;
        closestHit = collision.distance;
      }
    }
  }
  return selectedId;
}

void EntitySelectedAdd(short selectedId, EntityList *entityList)
{
  for (int i = 0; i < GAME_MAX_SELECTED; i++)
  {
    if (entityList->selected[i] == selectedId)
    {
      entityList->selected[i] = -1;
      break;
    }
    if (entityList->selected[i] == -1)
    {
      entityList->selected[i] = selectedId;
      break;
    }
  }
}

void EntitySelectedRemoveAll(EntityList *entityList)
{
  memset(entityList->selected, -1, sizeof(entityList->selected));
}

void EntitySelectedRemove(short selectedId, EntityList *entityList)
{
  for (int i = 0; i < GAME_MAX_SELECTED; i++)
  {
    if (entityList->selected[i] == selectedId)
    {
      entityList->selected[i] = -1;
      break;
    }
  }
}