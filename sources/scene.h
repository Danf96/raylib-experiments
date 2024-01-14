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

#define GAME_TEAM_PLAYER 1
#define GAME_TEAM_AI 2

typedef enum game_entity_type
{
  GAME_ENT_TYPE_EMPTY = 0,
  GAME_ENT_TYPE_ACTOR,
  GAME_ENT_TYPE_OBJECT,
} game_entity_type;

typedef enum game_entity_state
{
  GAME_ENT_STATE_IDLE = 0,
  GAME_ENT_STATE_MOVING = (1<<1),
  GAME_ENT_STATE_ATTACKING = (1<<2),
  GAME_ENT_STATE_DEAD = (1<<3),
  GAME_ENT_STATE_ACTION = (1<<4),
} game_entity_state;

// stores model information, and all animations with count, coult later refactor into structure of arrays
typedef struct
{
  uint16_t id;
  uint8_t team;
  uint8_t anim_index;
  bool is_dirty;
  uint32_t anim_current_frame;
  int32_t anims_count;
  Model model;
  ModelAnimation *anim;
  float offset_y;
  Vector3 scale;
  Vector3 position;
  Vector3 dimensions;
  Vector3 dimensions_offset;
  Vector3 rotation;
  Vector2 target_pos;
  uint16_t target_id;
  float move_speed;
  float attack_radius;
  float attack_damage;
  float attack_cooldown_max;
  float attack_cooldown;
  float hit_points;
  float hit_points_max;
  game_entity_type type;
  BoundingBox bbox;
  game_entity_state state;
} game_entity_t;

typedef struct
{
  Vector3 scale;
  uint8_t team;
  Vector2 position;
  Vector3 dimensions;
  Vector3 dimensions_offset;
  Vector3 rotation;
  float offset_y;
  float move_speed;
  float attack_radius;
  float attack_damage;
  float attack_cooldown_max;
  float hit_points;
  game_entity_type type;
  char *model_path;
  char *model_anims_path;
} game_entity_create_t;


game_entity_t * entity_add(game_entity_t entities[], game_entity_create_t *entity_create);

void entity_update_all(game_camera_t *camera, game_entity_t entities[], game_terrain_map_t *terrain_map, short selected[GAME_MAX_SELECTED], float dt);

BoundingBox entity_bbox_derive(Vector3 *position, Vector3 *dimensions_offset, Vector3 *dimensions);

void entity_bbox_update(Vector3 position, BoundingBox *bbox);

void entity_set_moving(Vector2 position, short entity_id, game_entity_t *entities);

void entity_set_attacking(uint16_t target_id, game_entity_t *entities, short selected[GAME_MAX_SELECTED]);


bool entity_check_attack(game_entity_t *ent, game_entity_t entities[]);

void entity_resolve_attack(game_entity_t *ent, game_entity_t entities[]);

void entity_add_selected(short selected_id, short selected[GAME_MAX_SELECTED]);

void entity_remove_selected_all(short selected[GAME_MAX_SELECTED]);

void entity_remove_selected(short selected_id, short selected[GAME_MAX_SELECTED]);

short entity_get_id(Ray ray, game_entity_t entities[]);

void entity_dirty_update(Vector3 old_pos, game_entity_t *ent, game_terrain_map_t *terrain_map);

void entity_collision_check(game_entity_t *ent, game_entity_t entities[]);

void entity_unload_all(game_entity_t entities[]);