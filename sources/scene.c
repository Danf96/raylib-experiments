#include "scene.h"
#include <string.h>
#include "stb_ds.h"

static size_t GLOBAL_ID = 0;

game_entity_t * entity_add(game_entity_t entities[], game_entity_create_t *entity_create)
{
  game_entity_t entity = (game_entity_t){.position = (Vector3){entity_create->position.x, entity_create->offset_y, entity_create->position.y},
                           .offset_y = entity_create->offset_y,
                           .rotation = entity_create->rotation,
                           .dimensions = entity_create->dimensions,
                           .dimensions_offset = entity_create->dimensions_offset,
                           .scale = entity_create->scale,
                           .type = entity_create->type,
                           .id = GLOBAL_ID,
                           .move_speed = entity_create->move_speed,
                           .is_dirty = true,
                           .anim_index = 2, // idle for the robot gltf
                           .team = entity_create->team,
                           };
  entity.model = LoadModel(entity_create->model_path);
  entity.anim = LoadModelAnimations(entity_create->model_anims_path, &entity.anims_count);
  entity.bbox = entity_bbox_derive(&entity.position, &entity.dimensions_offset, &entity.dimensions);
  arrput(entities, entity);
  GLOBAL_ID++;
  return entities;
}

void entity_unload_all(game_entity_t entities[])
{
  for (int i = 0; i < arrlen(entities); i++)
  {
    UnloadModel(entities[i].model);
    UnloadModelAnimations(entities[i].anim, entities[i].anims_count);
  }
  arrfree(entities);
}

void entity_update_all(game_camera_t *camera, game_entity_t entities[], game_terrain_map_t *terrain_map, short selected[GAME_MAX_SELECTED])
{
  // iterate over entitylist here for attacking entities, check attack ranges
  if (camera->focused)
  {
    Ray ray = GetMouseRay(GetMousePosition(), camera->ray_view_cam); // we should be always getting mouse data (can change to attack icon later when mousing over enemy)
    if (camera->is_button_pressed && camera->click_timer <= 0)
    {
      if (camera->mouse_button == MOUSE_BUTTON_LEFT)
      {
        short id = entity_get_id(ray, entities);
        if (id >= 0)
        {
          if (camera->modifier_key & ADDITIONAL_MODIFIER)
          {
            entity_add_selected(id, selected);
          }
          else
          {
            entity_remove_selected_all(selected);
            entity_add_selected(id, selected);
          }
        }
        else
        {
          entity_remove_selected_all(selected);
        }
      }
      else if (camera->mouse_button == MOUSE_BUTTON_RIGHT)
      {

        if (camera->modifier_key & ATTACK_MODIFIER)
        {
          // TODO: write function for attacking, maybe getting the entity ID of the target so that target positions can be updated as one entity moves over
        }
        else
        {
          Vector3 target = terrain_get_ray(ray, terrain_map, camera->near_plane, camera->far_plane);
          for (int i = 0; i < GAME_MAX_SELECTED; i++)
          {
            if (selected[i] >= 0)
            {
              entity_set_moving((Vector2){target.x, target.z}, selected[i], entities);
            }
          }
        }
      }
      camera->click_timer = 0.2f; // add delay to input
    }
  }
  for (size_t i = 0; i < arrlen(entities); i++)
  {
    // update entities here then mark dirty
    game_entity_t *ent = &entities[i];
    Vector3 old_pos = ent->position;
    if (ent->state & GAME_ENT_STATE_MOVING)
    {
      if (Vector2Equals((Vector2){ent->position.x, ent->position.z}, ent->target_pos))
      {
        ent->state ^= GAME_ENT_STATE_MOVING;
        ent->anim_index = 2;
      }
      else
      {
        ent->anim_index = 10; // may need to adjust with collision check later
        float adjusted_speed = ent->move_speed;
        Vector2 raw_dist = Vector2Subtract(ent->target_pos, (Vector2){ent->position.x, ent->position.z});
        Vector2 move_vec = Vector2Scale(Vector2Normalize(raw_dist), adjusted_speed);
        if (Vector2Length(raw_dist) < Vector2Length(move_vec))
        {
          move_vec = raw_dist;
        }
        // check collisions based purely on positions, keep bbox for only mouse selections
        // rotations not working correctly
        Vector3 newPos = Vector3Add((Vector3){move_vec.x, 0.0, move_vec.y}, ent->position);
        ent->rotation.y = (float)atan2(move_vec.x, move_vec.y);
        ent->position = newPos;
        // position will be adjusted within EntityCheckCollision
        entity_collision_check(ent, entities);
        ent->is_dirty = true;
      }
    }
    ent->anim_current_frame = (ent->anim_current_frame + 1) % ent->anim[ent->anim_index].frameCount;
    UpdateModelAnimation(ent->model, ent->anim[ent->anim_index], ent->anim_current_frame);
    if (ent->is_dirty)
    {
      entity_dirty_update(old_pos, ent, terrain_map);
    }
  }
}

void entity_dirty_update(Vector3 old_pos, game_entity_t *ent, game_terrain_map_t *terrain_map)
{
  Vector3 adjusted_pos = (Vector3){.x = ent->position.x,
                                  .z = ent->position.z,
                                  .y = ent->offset_y + terrain_get_adjusted_y(ent->position, terrain_map)};
  ent->position = adjusted_pos;
  ent->model.transform = MatrixMultiply(MatrixRotateZYX(ent->rotation),
                                    MatrixMultiply(MatrixTranslate(adjusted_pos.x, adjusted_pos.y, adjusted_pos.z),
                                                   MatrixScale(ent->scale.x, ent->scale.y, ent->scale.z)));
  
  entity_bbox_update(Vector3Subtract(adjusted_pos, old_pos), &ent->bbox);
  ent->is_dirty = false;
}

BoundingBox entity_bbox_derive(Vector3 *position, Vector3 *dimensions_offset, Vector3 *dimensions)
{
    return (BoundingBox){
      (Vector3){position->x - (dimensions->x) / 2.0f + dimensions_offset->x,
                position->y - (dimensions->y) / 2.0f + dimensions_offset->y,
                position->z - (dimensions->z) / 2.0f + dimensions_offset->z},
      (Vector3){position->x + (dimensions->x) / 2.0f + dimensions_offset->x,
                position->y + (dimensions->y) / 2.0f + dimensions_offset->y,
                position->z + (dimensions->z) / 2.0f + dimensions_offset->z},
  };
}

void entity_bbox_update(Vector3 position, BoundingBox *bbox)
{
  bbox->max = Vector3Add(bbox->max, position);
  bbox->min = Vector3Add(bbox->min, position);
}

void entity_set_moving(Vector2 position, short entity_id, game_entity_t *entities)
{
  // used purely for move orders, negates attack
  game_entity_t *target_ent = &entities[entity_id];
  target_ent->target_pos = position;
  target_ent->state |= GAME_ENT_STATE_MOVING;
}

short entity_get_id(Ray ray, game_entity_t entities[])
{
  float closest_hit = __FLT_MAX__;
  short selected_id = -1;
  for (size_t i = 0; i < arrlen(entities); i++)
  {
    RayCollision collision = GetRayCollisionBox(ray, entities[i].bbox);
    if (collision.hit)
    {
      if (collision.distance < closest_hit)
      {
        selected_id = entities[i].id;
        closest_hit = collision.distance;
      }
    }
  }
  return selected_id;
}

void entity_add_selected(short selected_id, short selected[GAME_MAX_SELECTED])
{
  for (int i = 0; i < GAME_MAX_SELECTED; i++)
  {
    if (selected[i] == selected_id)
    {
      selected[i] = -1;
      break;
    }
    if (selected[i] == -1)
    {
      selected[i] = selected_id;
      break;
    }
  }
}

void entity_remove_selected_all(short selected[GAME_MAX_SELECTED])
{
  memset(selected, -1, sizeof(*selected * GAME_MAX_SELECTED));
}

void entity_remove_selected(short selectedId, short selected[GAME_MAX_SELECTED])
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
void entity_collision_check(game_entity_t *ent, game_entity_t entities[])
{
  
  Rectangle source_rec = (Rectangle){.x = ent->bbox.min.x,
                                    .y = ent->bbox.min.z,
                                    .width = ent->bbox.max.x - ent->bbox.min.x,
                                    .height = ent->bbox.max.z- ent->bbox.min.z};

  for (int i = 0; i < arrlen(entities); i++) 
  {
    game_entity_t *target_ent = &entities[i];
    if (ent->id == target_ent->id) continue;
    Rectangle target_rec = (Rectangle){.x = target_ent->bbox.min.x,
                                    .y = target_ent->bbox.min.z,
                                    .width = target_ent->bbox.max.x - target_ent->bbox.min.x,
                                    .height = target_ent->bbox.max.z- target_ent->bbox.min.z};
    bool collision = CheckCollisionRecs(source_rec, target_rec);
    if (collision) 
    {
      Rectangle collision_rec = GetCollisionRec(source_rec, target_rec);
      // if width smaller than height, move entity on X axis (pick shortest intersection)
      if (collision_rec.width < collision_rec.height) {
        float direction = ent->position.x < collision_rec.x ? -1 : 1;
        ent->position.x += (collision_rec.width) * direction;
      }
      else
      {
        float direction = ent->position.z < collision_rec.y ? -1 : 1;
        ent->position.z += (collision_rec.height) * direction;
      }
    }
  }
}