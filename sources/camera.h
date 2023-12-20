#pragma once

#include "raylib.h"
#include "raymath.h"
#include "terrain.h"
#include "stdint.h"
// Using Jeff M's Raylib extras camera
// https://github.com/raylib-extras/extras-c/tree/main/cameras
typedef enum
{
  MOVE_FRONT = 0,
  MOVE_BACK,
  MOVE_RIGHT,
  MOVE_LEFT,
  MOVE_UP,
  MOVE_DOWN,
  ROTATE_RIGHT,
  ROTATE_LEFT,
  ROTATE_UP,
  ROTATE_DOWN,
  MODIFIER_1,
  MODIFIER_2,
  LAST_CONTROL
} game_camera_controls;

typedef enum
{
  ADDITIONAL_MODIFIER = (1<<0),
  ATTACK_MODIFIER = (1<<1),
} game_camera_actions;

typedef struct
{
  int controls_keys[LAST_CONTROL];

  uint8_t mouse_button;
  uint8_t modifier_key;
  bool is_button_pressed;

  // the speed in units/second to move
  Vector3 move_speed;

  Vector2 rotation_speed;

  float mouse_sens;

  float click_timer;

  float min_view_y;
  float max_view_y;

  Vector3 camera_pos;

  float camera_pullback_dist;

  Camera ray_view_cam;


  Vector2 fov;

  Vector2 view_angles;

  bool focused;

  double near_plane;
  double far_plane;

} game_camera_t;

void game_camera_init(game_camera_t *camera, float fov_y, Vector3 position, game_terrain_map_t *terrain_map);

Vector3 game_camera_get_world_pos(game_camera_t *camera);

// Vector3 game_camera_get_terrain_pos(game_camera_t *camera);

void game_camera_set_pos(game_camera_t *camera, Vector3 position);

Ray game_camera_get_view_ray(game_camera_t *camera);

void game_camera_update(game_camera_t *camera, game_terrain_map_t *terrain_map);

void game_camera_begin_mode_3d(game_camera_t *camera);

void game_camera_end_mode_3d(void);

void game_camera_input_clear(game_camera_t *camera);
