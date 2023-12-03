#pragma once

#include "raylib.h"
#include "raymath.h"
#include "model.h"
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
  SPRINT,
  LAST_CONTROL
} RTSCameraControls;

typedef struct
{
  int ControlsKeys[LAST_CONTROL];

  // the speed in units/second to move
  Vector3 MoveSpeed;

  Vector2 RotationSpeed;

  bool UseMouse;
  int UseMouseButton;

  float MouseSensitivity;

  float MinimumViewY;
  float MaximumViewY;

  Vector3 CameraPosition;

  float CameraPullbackDistance;

  Camera ViewCamera;

  Vector3 ViewForward;

  Vector2 FOV;

  Vector2 ViewAngles;

  bool Focused;

  double NearPlane;
  double FarPlane;

} RTSCamera;

void RTSCameraInit(RTSCamera *camera, float fovY, Vector3 position);

void RTSCameraUseMouse(RTSCamera *camera, bool useMouse, int button);

Vector3 RTSCameraGetPosition(RTSCamera *camera);

void RTSCameraSetPosition(RTSCamera *camera, Vector3 position);

Ray RTSCameraGetViewRay(RTSCamera *camera);

void RTSCameraUpdate(RTSCamera *camera);

void RTSCameraBeginMode3D(RTSCamera *camera);

void RTSCameraEndMode3D(void);

void RTSCameraAdjustHeight(RTSCamera *camera, HeightMap *heightMap);