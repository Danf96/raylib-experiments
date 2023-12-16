#include "scene.h"
#include <string.h>
#include "stb_ds.h"

static size_t GLOBAL_ID = 0;

Entity* AddEntity(Entity *entities, EntityCreate *entityCreate)
{
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
  arrpush(entities, entity);
  GLOBAL_ID++;
  return entities;
}



void UpdateEntities(RTSCamera *camera, Entity *entities, TerrainMap *terrainMap, short *selected)
{
  // iterate over entitylist here for attacking entities, check attack ranges
  if (camera->Focused)
  {
    Ray ray = GetMouseRay(GetMousePosition(), camera->ViewCamera); // we should be always getting mouse data (can change to attack icon later when mousing over enemy)
    if (camera->IsButtonPressed && camera->ClickTimer <= 0)
    {
      if (camera->MouseButton == MOUSE_BUTTON_LEFT)
      {
        short id = EntityGetSelectedId(ray, entities);
        if (id >= 0)
        {
          if (camera->ModifierKey & ADDITIONAL_MODIFIER)
          {
            EntitySelectedAdd(id, selected);
          }
          else
          {
            EntitySelectedRemoveAll(selected);
            EntitySelectedAdd(id, selected);
          }
        }
        else
        {
          EntitySelectedRemoveAll(selected);
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
            if (selected[i] >= 0)
            {
              EntitySetMoving((Vector2){target.x, target.z}, selected[i], entities);
            }
          }
        }
      }
      camera->ClickTimer = 0.2f; // add delay to input
    }
  }
  for (size_t i = 0; i < arrlen(entities); i++)
  {
    // update entities here then mark dirty
    Entity *ent = &entities[i];
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
        // check collisions based purely on positions, keep bbox for only mouse selections
        // rotations not working correctly
        Vector3 newPos = Vector3Add((Vector3){moveVec.x, 0.0, moveVec.y}, ent->position);
        ent->rotation.y = (float)atan2(moveVec.x, moveVec.y);
        ent->position = newPos;
        // position will be adjusted within EntityCheckCollision
        EntityCheckCollision(ent, entities);
        ent->isDirty = true;
      }
    }

    if (ent->isDirty)
    {
      EntityUpdateDirty(ent, terrainMap);
    }
  }
}
void EntityUpdateDirty(Entity *ent, TerrainMap *terrainMap)
{
  Vector3 adjustedPos = (Vector3){.x = ent->position.x,
                                  .z = ent->position.z,
                                  .y = ent->offsetY + GetAdjustedHeight(ent->position, terrainMap)};
  ent->position = adjustedPos;
  ent->worldMatrix = MatrixMultiply(MatrixRotateZYX(ent->rotation),
                                    MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                                                   MatrixScale(ent->scale.x, ent->scale.y, ent->scale.z)));
  ent->bbox = EntityBBoxDerive(&adjustedPos, &ent->dimensions, &ent->scale);
  ent->isDirty = false;
}
BoundingBox EntityBBoxDerive(Vector3 *position, Vector3 *dimensions, Vector3 *scale)
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



void EntitySetMoving(Vector2 position, short entityId, Entity *entities)
{
  // used purely for move orders, negates attack
  Entity *targetEntity = &entities[entityId];
  targetEntity->targetPos = position;
  targetEntity->state |= ENT_IS_MOVING;
}

short EntityGetSelectedId(Ray ray, Entity *entities)
{
  float closestHit = __FLT_MAX__;
  short selectedId = -1;
  for (size_t i = 0; i < arrlen(entities); i++)
  {
    RayCollision collision = GetRayCollisionBox(ray, entities[i].bbox);
    if (collision.hit)
    {
      if (collision.distance < closestHit)
      {
        selectedId = entities[i].id;
        closestHit = collision.distance;
      }
    }
  }
  return selectedId;
}

void EntitySelectedAdd(short selectedId, short *selected)
{
  for (int i = 0; i < GAME_MAX_SELECTED; i++)
  {
    if (selected[i] == selectedId)
    {
      selected[i] = -1;
      break;
    }
    if (selected[i] == -1)
    {
      selected[i] = selectedId;
      break;
    }
  }
}

void EntitySelectedRemoveAll(short *selected)
{
  memset(selected, -1, sizeof(*selected * GAME_MAX_SELECTED));
}

void EntitySelectedRemove(short selectedId, short *selected)
{
  for (int i = 0; i < GAME_MAX_SELECTED; i++)
  {
    if (selected[i] == selectedId)
    {
      selected[i] = -1;
      break;
    }
  }
}
void EntityCheckCollision(Entity *ent, Entity *entities)
{
  // Do not use bbox for these calculations, those only get updated when entity is marked dirty
  Rectangle sourceRec = (Rectangle){.x = ent->position.x - (ent->dimensions.x / 2.0f),
                                    .y = ent->position.z - (ent->dimensions.z / 2.0f), 
                                    .width = ent->dimensions.x, 
                                    .height = ent->dimensions.z};

  for (int i = 0; i < arrlen(entities); i++) 
  {
    Entity *targetEnt = &entities[i];
    if (ent->id == targetEnt->id) continue;
    Rectangle targetRec = (Rectangle){.x = targetEnt->position.x - (targetEnt->dimensions.x / 2.0f),
                                      .y = targetEnt->position.z - (targetEnt->dimensions.z / 2.0f), 
                                      .width = targetEnt->dimensions.x, 
                                      .height = targetEnt->dimensions.z};
    bool collision = CheckCollisionRecs(sourceRec, targetRec);
    if (collision) 
    {
      Rectangle collisionRec = GetCollisionRec(sourceRec, targetRec);
      // if width smaller than height, move entity on X axis (pick shortest intersection)
      if (collisionRec.width < collisionRec.height) {
        float direction = ent->position.x < collisionRec.x ? -1 : 1;
        ent->position.x += (collisionRec.width) * direction;
      }
      else
      {
        float direction = ent->position.z < collisionRec.y ? -1 : 1;
        ent->position.z += (collisionRec.height) * direction;
      }
    }
  }
}