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
                           .dimensionsOffset = entityCreate->dimensionsOffset,
                           .scale = entityCreate->scale,
                           .type = entityCreate->type,
                           .id = GLOBAL_ID,
                           .moveSpeed = entityCreate->moveSpeed,
                           .isDirty = true,
                           .animIndex = 2,
                           };
  entity.model = LoadModel(entityCreate->modelPath);
  entity.anim = LoadModelAnimations(entityCreate->modelAnimsPath, &entity.animsCount);
  entity.bbox = EntityBBoxDerive(&entity.position, &entity.dimensionsOffset, &entity.dimensions);
  arrput(entities, entity);
  GLOBAL_ID++;
  return entities;
}

void UnloadEntities(Entity *entities)
{
  for (int i = 0; i < arrlen(entities); i++)
  {
    UnloadModel(entities[i].model);
    UnloadModelAnimations(entities[i].anim, entities[i].animsCount);
  }
  arrfree(entities);
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
    Vector3 oldPos = ent->position;
    if (ent->state & ENT_IS_MOVING)
    {
      if (Vector2Equals((Vector2){ent->position.x, ent->position.z}, ent->targetPos))
      {
        ent->state ^= ENT_IS_MOVING;
        ent->animIndex = 2;
      }
      else
      {
        ent->animIndex = 10; // may need to adjust with collision check later
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
    ent->animCurrentFrame = (ent->animCurrentFrame + 1) % ent->anim[ent->animIndex].frameCount;
    UpdateModelAnimation(ent->model, ent->anim[ent->animIndex], ent->animCurrentFrame);
    if (ent->isDirty)
    {
      EntityUpdateDirty(oldPos, ent, terrainMap);
    }
  }
}
void EntityUpdateDirty(Vector3 oldPos, Entity *ent, TerrainMap *terrainMap)
{
  Vector3 adjustedPos = (Vector3){.x = ent->position.x,
                                  .z = ent->position.z,
                                  .y = ent->offsetY + GetAdjustedHeight(ent->position, terrainMap)};
  ent->position = adjustedPos;
  ent->model.transform = MatrixMultiply(MatrixRotateZYX(ent->rotation),
                                    MatrixMultiply(MatrixTranslate(adjustedPos.x, adjustedPos.y, adjustedPos.z),
                                                   MatrixScale(ent->scale.x, ent->scale.y, ent->scale.z)));
  
  EntityBBoxUpdate(Vector3Subtract(adjustedPos, oldPos), &ent->bbox);
  ent->isDirty = false;
}

BoundingBox EntityBBoxDerive(Vector3 *position, Vector3 *dimensionsOffset, Vector3 *dimensions)
{
    return (BoundingBox){
      (Vector3){position->x - (dimensions->x) / 2.0f + dimensionsOffset->x,
                position->y - (dimensions->y) / 2.0f + dimensionsOffset->y,
                position->z - (dimensions->z) / 2.0f + dimensionsOffset->z},
      (Vector3){position->x + (dimensions->x) / 2.0f + dimensionsOffset->x,
                position->y + (dimensions->y) / 2.0f + dimensionsOffset->y,
                position->z + (dimensions->z) / 2.0f + dimensionsOffset->z},
  };
}

void EntityBBoxUpdate(Vector3 position, BoundingBox *bbox)
{
  bbox->max = Vector3Add(bbox->max, position);
  bbox->min = Vector3Add(bbox->min, position);
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

/**
 * @brief Constructs 2D rectangles from entity's bounding box for collision checks
 * 
 * @param ent source entity to check against collisions
 * @param entities list of entities to iterate over
 */
void EntityCheckCollision(Entity *ent, Entity *entities)
{
  
  Rectangle sourceRec = (Rectangle){.x = ent->bbox.min.x,
                                    .y = ent->bbox.min.z,
                                    .width = ent->bbox.max.x - ent->bbox.min.x,
                                    .height = ent->bbox.max.z- ent->bbox.min.z};

  for (int i = 0; i < arrlen(entities); i++) 
  {
    Entity *targetEnt = &entities[i];
    if (ent->id == targetEnt->id) continue;
    Rectangle targetRec = (Rectangle){.x = targetEnt->bbox.min.x,
                                    .y = targetEnt->bbox.min.z,
                                    .width = targetEnt->bbox.max.x - targetEnt->bbox.min.x,
                                    .height = targetEnt->bbox.max.z- targetEnt->bbox.min.z};
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