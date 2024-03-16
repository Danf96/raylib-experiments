#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "stb_ds.h"

#include "terrain.h"
#include "util.h"

// defined based on a reasonable zoom level, subject to change
#define MOUSE_MINIMUM_AREA (150.f)

extern bool is_select_visible;

static void game_camera_resize_view(game_camera_t *camera)
{
  if (!camera)
  {
    return;
  }

  float width = (float)GetScreenWidth();
  float height = (float)GetScreenHeight();

  camera->fov.y = camera->ray_view_cam.fovy;

  if (height != 0)
    camera->fov.x = camera->fov.y * (width / height);
}

static void game_camera_get_mouse_states(game_camera_t *camera)
{
  for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_RIGHT; button++)
  {
    if (IsMouseButtonDown(button))
    {
      camera->mouse_states |= (1 << (button));
    }
  }
}
static void game_camera_detect_mouse_events(game_camera_t *camera) 
{
  uint8_t mouse_changes = camera->mouse_states ^ camera->prev_mouse_states;

  camera->mouse_downs = mouse_changes & camera->mouse_states;
  camera->mouse_ups = mouse_changes & (~camera->mouse_states);
}

static float game_camera_get_mouse_area(Vector2 pos_new, Vector2 pos_old)
{
  double x = fabs(pos_new.x - pos_old.x);
  double y = fabs(pos_new.y - pos_old.y);
  return (float)(x * y);
}

// solve for top left corner, then get dimensions based on absolute value difference
Rectangle game_camera_get_mouse_rect(Vector2 pos_old, Vector2 pos_new)
{
  Rectangle result = {0};
  if (pos_old.x < pos_new.x)
  {
    result.x = pos_old.x;
  }
  else
  {
    result.x = pos_new.x;
  }
  if (pos_old.y < pos_new.y)
  {
    result.y = pos_old.y;
  }
  else
  {
    result.y = pos_new.y;
  }
  result.width = fabs(pos_new.x - pos_old.x);
  result.height = fabs(pos_new.y - pos_old.y);
  return result;
}

static void game_camera_create_input_events(game_camera_t *camera)
{
  bool is_new_event = false;
  game_input_event_t new_event = {0};
  if (camera->mouse_downs)
  {
    if (camera->mouse_downs & (1 << MOUSE_BUTTON_LEFT))
    {
      Vector2 mouse_pos = GetMousePosition();
      if(IsKeyDown(camera->controls_keys[MODIFIER_ATTACK]))
      {
        is_select_visible = false;
        new_event.event_type = LEFT_CLICK_ATTACK;
        new_event.mouse_ray = GetMouseRay(mouse_pos, camera->ray_view_cam);
        is_new_event = true;
      }
      else
      {
        is_select_visible = true; // toggle global for UI
        camera->mouse_old_pos = mouse_pos;
      }
    }
    if (camera->mouse_downs & (1 << MOUSE_BUTTON_RIGHT))
    {
      new_event.event_type = RIGHT_CLICK;
      new_event.mouse_ray = GetMouseRay(GetMousePosition(), camera->ray_view_cam);
      is_new_event = true;
    }
  } 
  if (camera->mouse_ups)
  {
    // only tracking left mouse button mouse up events currently, right does not have any special states at the moment
    if (camera->mouse_ups & (1 << MOUSE_BUTTON_LEFT))
    {
      is_select_visible = false; // toggle global for rectangle drawing
      if (IsKeyDown(camera->controls_keys[MODIFIER_ADD]))
      {
        Vector2 new_pos = GetMousePosition();
        if (game_camera_get_mouse_area(new_pos, camera->mouse_old_pos) > MOUSE_MINIMUM_AREA)
        {
          new_event.event_type = LEFT_CLICK_ADD_GROUP;
          new_event.mouse_rect = game_camera_get_mouse_rect(camera->mouse_old_pos, new_pos);
        }
        else
        {
          new_event.event_type = LEFT_CLICK_ADD;
          new_event.mouse_ray = GetMouseRay(new_pos, camera->ray_view_cam);
        }
        is_new_event = true;
      }
      else if (!IsKeyDown(camera->controls_keys[MODIFIER_ATTACK]))
      {
        // only works with current number of modifiers since there are only three at the moment
        Vector2 new_pos = GetMousePosition();
        if (game_camera_get_mouse_area(new_pos, camera->mouse_old_pos) > MOUSE_MINIMUM_AREA)
        {
          new_event.event_type = LEFT_CLICK_GROUP;
          new_event.mouse_rect = game_camera_get_mouse_rect(camera->mouse_old_pos, new_pos);
        }
        else 
        {
          new_event.event_type = LEFT_CLICK;
          new_event.mouse_ray = GetMouseRay(new_pos, camera->ray_view_cam);
        }
        is_new_event = true;
      }
    }
  }
  if (is_new_event)
  {
      arrput(camera->input_events, new_event);
  }
}

void game_camera_init(game_camera_t *camera, float fov_y, Vector3 position, game_terrain_map_t *terrain_map)
{
  if (!camera)
  {
    return;
  }
    
  camera->controls_keys[0] = 'W';                   // MOVE_FRONT
  camera->controls_keys[1] = 'S';                   // MOVE_BACK
  camera->controls_keys[2] = 'D';                   // MOVE_RIGHT
  camera->controls_keys[3] = 'A';                   // MOVE_LEFT
  camera->controls_keys[4] = KEY_SPACE;             // MOVE_UP
  camera->controls_keys[5] = KEY_LEFT_CONTROL;      // MOVE_DOWN
  camera->controls_keys[6] = 'E';                   // ROTATE_RIGHT
  camera->controls_keys[7] = 'Q';                   // ROTATE_LEFT
  camera->controls_keys[8] = KEY_UP;                // ROTATE_UP
  camera->controls_keys[9] = KEY_DOWN;              // ROTATE_DOWN
  camera->controls_keys[10] = KEY_LEFT_SHIFT;       // MODIFIER_ADD
  camera->controls_keys[11] = KEY_LEFT_CONTROL;     // MODIFIER_ATTACK

  camera->mouse_states = 0;
  camera->move_speed = (Vector3){8, 8, 8};
  camera->rotation_speed = (Vector2){90, 90};

  camera->mouse_sens = 600;

  camera->min_view_y = -50.0f;
  camera->max_view_y = -15.0f;

  camera->input_events = NULL;


  camera->focused = IsWindowFocused();

  camera->camera_pullback_dist= 15.f;

  camera->view_angles = (Vector2){180.f * DEG2RAD, -45.f * DEG2RAD};

  camera->camera_pos = position;
  camera->camera_pos.y = terrain_get_adjusted_y(camera->camera_pos, terrain_map);
  camera->fov.y = fov_y;

  camera->ray_view_cam.target = position;
  camera->ray_view_cam.position = Vector3Add(camera->ray_view_cam.target, (Vector3){0, 0, camera->camera_pullback_dist});
  camera->ray_view_cam.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera->ray_view_cam.fovy = fov_y;
  camera->ray_view_cam.projection = CAMERA_PERSPECTIVE;

  camera->near_plane = 0.01;
  camera->far_plane = 1000.0;

  game_camera_resize_view(camera);
}

Vector3 game_camera_get_world_pos(game_camera_t *camera)
{
  // uses world coordinates, currently unused
  return camera->camera_pos;
}

void game_camera_set_pos(game_camera_t *camera, Vector3 position)
{
  // uses world coordinates
  camera->camera_pos = position;
}

Ray game_camera_get_view_ray(game_camera_t *camera)
{
  return (Ray){camera->ray_view_cam.position, Vector3Subtract(camera->ray_view_cam.target, camera->ray_view_cam.position)};
}

static float game_camera_get_axis_speed(game_camera_t *camera, game_camera_controls axis, float speed, float dt)
{
  if (!camera)
  {
    return 0;
  }

  int key = camera->controls_keys[axis];
  if (key == -1)
  {
    return 0;
  }

  float factor = 1.0f;

  if (IsKeyDown(camera->controls_keys[axis]))
    return speed * dt * factor;

  return 0.0f;
}

void game_camera_update(game_camera_t *camera, game_terrain_map_t *terrain_map)
{
  if (!camera)
  {
    return;
  }
    
  if (IsWindowResized())
  {
    game_camera_resize_view(camera);
  }
    
  camera->focused = IsWindowFocused();


  if (camera->focused)
  {
    camera->prev_mouse_states = camera->mouse_states;
    camera->mouse_states = 0;
    game_camera_get_mouse_states(camera);
    game_camera_detect_mouse_events(camera);
    game_camera_create_input_events(camera);
  }

  Vector2 mouse_pos_delta = GetMouseDelta();
  float dt = GetFrameTime();
  float mouse_wheel_delta = GetMouseWheelMove();

  float direction[] = 
  {
      -game_camera_get_axis_speed(camera, MOVE_FRONT, camera->move_speed.z, dt),
      -game_camera_get_axis_speed(camera, MOVE_BACK, camera->move_speed.z, dt),
      game_camera_get_axis_speed(camera, MOVE_RIGHT, camera->move_speed.x, dt),
      game_camera_get_axis_speed(camera, MOVE_LEFT, camera->move_speed.x, dt)
  };

  bool rotate_mouse = (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON));
  if (rotate_mouse)
  {
    HideCursor();
  }
  else
  {
    ShowCursor();
  }

  float pivot_heading_rotation =
      game_camera_get_axis_speed(camera, ROTATE_RIGHT, camera->rotation_speed.x, dt) -
      game_camera_get_axis_speed(camera, ROTATE_LEFT, camera->rotation_speed.x, dt);
  float pivot_pitch_rotation =
      game_camera_get_axis_speed(camera, ROTATE_UP, camera->rotation_speed.y, dt) -
      game_camera_get_axis_speed(camera, ROTATE_DOWN, camera->rotation_speed.y, dt);

  if (pivot_heading_rotation)
    camera->view_angles.x -= pivot_heading_rotation * DEG2RAD;
  else if (rotate_mouse && camera->focused)
    camera->view_angles.x -= (mouse_pos_delta.x / camera->mouse_sens);

  if (pivot_pitch_rotation)
    camera->view_angles.y += pivot_pitch_rotation * DEG2RAD;
  else if (rotate_mouse && camera->focused)
    camera->view_angles.y += (mouse_pos_delta.y / -camera->mouse_sens);

  // Clamp view angles
  if (camera->view_angles.y < camera->min_view_y * DEG2RAD)
    camera->view_angles.y = camera->min_view_y * DEG2RAD;
  else if (camera->view_angles.y > camera->max_view_y * DEG2RAD)
    camera->view_angles.y = camera->max_view_y * DEG2RAD;

  Vector3 move_vec = {0};
  move_vec.x = direction[MOVE_RIGHT] - direction[MOVE_LEFT];
  move_vec.z = direction[MOVE_FRONT] - direction[MOVE_BACK];

  // Update zoom
  camera->camera_pullback_dist -= (mouse_wheel_delta);
  camera->camera_pullback_dist = clamp_float(camera->camera_pullback_dist, 1, (camera->far_plane / 4));
    

  Vector3 cam_pos = {.z = camera->camera_pullback_dist};

  Matrix tilt_mat = MatrixRotateX(camera->view_angles.y);
  Matrix rot_mat = MatrixRotateY(camera->view_angles.x);
  Matrix mat = MatrixMultiply(tilt_mat, rot_mat);

  cam_pos = Vector3Transform(cam_pos, mat);
  move_vec = Vector3Transform(move_vec, rot_mat);

  camera->camera_pos = Vector3Add(camera->camera_pos, move_vec);
  camera->camera_pos.y = terrain_get_adjusted_y(camera->camera_pos, terrain_map);
  camera->ray_view_cam.target = camera->camera_pos;
  camera->ray_view_cam.position = Vector3Add(camera->camera_pos, cam_pos);

  camera->click_timer -= dt;
}

static void game_camera_setup(game_camera_t *camera, float aspect)
{
  rlDrawRenderBatchActive();
  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();

  if (camera->ray_view_cam.projection == CAMERA_PERSPECTIVE)
  {
    double top =
        RL_CULL_DISTANCE_NEAR * tan(camera->ray_view_cam.fovy * 0.5 * DEG2RAD);
    double right = top * aspect;

    rlFrustum(-right, right, -top, top, camera->near_plane, camera->far_plane);
  }
  else if (camera->ray_view_cam.projection == CAMERA_ORTHOGRAPHIC)
  {
    double top = camera->ray_view_cam.fovy / 2.0;
    double right = top * aspect;

    rlOrtho(-right, right, -top, top, camera->near_plane, camera->far_plane);
  }

  rlMatrixMode(RL_MODELVIEW);
  rlLoadIdentity();

  Matrix matView =
      MatrixLookAt(camera->ray_view_cam.position, camera->ray_view_cam.target,
                   camera->ray_view_cam.up);

  rlMultMatrixf(MatrixToFloatV(matView).v);

  rlEnableDepthTest();
}

void game_camera_begin_mode_3d(game_camera_t *camera)
{
  if (!camera)
    return;

  float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
  game_camera_setup(camera, aspect);
}

void game_camera_end_mode_3d(void)
{
  EndMode3D();
}
void shadow_camera_calc(shadow_camera_t *camera)
{
  if (camera->ray_view_cam.projection == CAMERA_PERSPECTIVE)
  {
    camera->projection = MatrixPerspective(camera->ray_view_cam.fovy*DEG2RAD, 1.0, camera->near_plane, camera->far_plane);
  }
  else if (camera->ray_view_cam.projection == CAMERA_ORTHOGRAPHIC)
  {
    double top = camera->ray_view_cam.fovy / 2.0;
    double right = top;

    camera->projection = MatrixOrtho(-right, right, -top, top, camera->near_plane, camera->far_plane);
  }
      
  camera->view = MatrixLookAt(camera->ray_view_cam.position, camera->ray_view_cam.target,
                   camera->ray_view_cam.up);
}

void shadow_camera_begin_mode_3d(shadow_camera_t *camera)
{
  if (!camera)
  {
    return;
  }
  rlDrawRenderBatchActive();

  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();

  
  rlMultMatrixf(MatrixToFloat(camera->projection));
  rlMatrixMode(RL_MODELVIEW);
  rlLoadIdentity();

  rlMultMatrixf(MatrixToFloat(camera->view));

  rlEnableDepthTest();
}

void shadow_camera_end_mode_3d(void)
{
  EndMode3D();
}

void shadow_camera_init(shadow_camera_t *camera, Vector3 position, float fovy, float near_plane, float far_plane, CameraProjection projection_type)
{
  if (!camera)
  {
    return;
  }
  camera->near_plane = near_plane;
  camera->far_plane = far_plane;
  camera->ray_view_cam = (Camera3D)
  {
    .fovy = fovy, 
    .position = position, 
    .up = (Vector3){0.f, 1.f, 0.f},
    .projection = projection_type,
  };

  shadow_camera_calc(camera);
}


