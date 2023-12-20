#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "raylib.h"
#include "rlgl.h"

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

void game_camera_init(game_camera_t *camera, float fov_y, Vector3 position, game_terrain_map_t *terrain_map)
{
  if (!camera)
    return;

  camera->controls_keys[0] = 'W';
  camera->controls_keys[1] = 'S';
  camera->controls_keys[2] = 'D';
  camera->controls_keys[3] = 'A';
  camera->controls_keys[4] = KEY_SPACE;
  camera->controls_keys[5] = KEY_LEFT_CONTROL;
  camera->controls_keys[6] = 'E';
  camera->controls_keys[7] = 'Q';
  camera->controls_keys[8] = KEY_UP;
  camera->controls_keys[9] = KEY_DOWN;
  camera->controls_keys[10] = KEY_LEFT_SHIFT;
  camera->controls_keys[11] = KEY_LEFT_CONTROL;

  camera->mouse_button = 0;
  camera->modifier_key = 0;

  camera->click_timer = 0;

  camera->move_speed = (Vector3){8, 8, 8};
  camera->rotation_speed = (Vector2){90, 90};

  camera->mouse_sens = 600;

  camera->min_view_y = -50.0f;
  camera->max_view_y = -15.0f;


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

static float game_camera_get_axis_speed(game_camera_t *camera, game_camera_controls axis, float speed)
{
  if (!camera)
    return 0;

  int key = camera->controls_keys[axis];
  if (key == -1)
    return 0;

  float factor = 1.0f;

  if (IsKeyDown(camera->controls_keys[axis]))
    return speed * GetFrameTime() * factor;

  return 0.0f;
}

void game_camera_update(game_camera_t *camera, game_terrain_map_t *terrain_map)
{
  if (!camera)
    return;

  if (IsWindowResized())
    game_camera_resize_view(camera);
  camera->focused = IsWindowFocused();

  bool is_pressed = false;

  if (camera->focused)
  {
    for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_MIDDLE; button++)
    {
      if (IsMouseButtonDown(button))
      {
        camera->mouse_button = button;
        camera->is_button_pressed = true;
        break;
      }
      else if (IsMouseButtonReleased(button))
      {
        camera->mouse_button = button;
        camera->is_button_pressed = false;
        break;
      }
    }
    if (IsKeyDown(camera->controls_keys[MODIFIER_1]))
    {
      camera->modifier_key = ADDITIONAL_MODIFIER;
    }
    else if (IsKeyDown(camera->controls_keys[MODIFIER_2]))
    {
      camera->modifier_key = ATTACK_MODIFIER;
    }
    else
    {
      camera->modifier_key = 0;
    }
  }
  else
    (camera->is_button_pressed = false); // no buttons registered when not focused

  Vector2 mouse_pos_delta = GetMouseDelta();
  // float mouseWheelMove = GetMouseWheelMove();

  float direction[MOVE_LEFT + 1] = {
      -game_camera_get_axis_speed(camera, MOVE_FRONT, camera->move_speed.z),
      -game_camera_get_axis_speed(camera, MOVE_BACK, camera->move_speed.z),
      game_camera_get_axis_speed(camera, MOVE_RIGHT, camera->move_speed.x),
      game_camera_get_axis_speed(camera, MOVE_LEFT, camera->move_speed.x)};

  bool rotate_mouse = (camera->mouse_button == MOUSE_BUTTON_MIDDLE && camera->is_button_pressed);
  if (rotate_mouse)
  {
    HideCursor();
  }
  else
  {
    ShowCursor();
  }

  float pivot_heading_rotation =
      game_camera_get_axis_speed(camera, ROTATE_RIGHT, camera->rotation_speed.x) -
      game_camera_get_axis_speed(camera, ROTATE_LEFT, camera->rotation_speed.x);
  float pivot_pitch_rotation =
      game_camera_get_axis_speed(camera, ROTATE_UP, camera->rotation_speed.y) -
      game_camera_get_axis_speed(camera, ROTATE_DOWN, camera->rotation_speed.y);

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
  camera->camera_pullback_dist += (-GetMouseWheelMove());
  if (camera->camera_pullback_dist < 1)
    camera->camera_pullback_dist = 1;

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

  camera->click_timer -= GetFrameTime();
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
