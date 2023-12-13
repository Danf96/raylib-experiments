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
  LAST_CONTROL
} RTSCameraControls;

enum RTSActions
{
  ADDITIONAL_MODIFIER = (1<<0),
  ATTACK_MODIFIER = (1<<1),
};

typedef struct
{
  int ControlsKeys[LAST_CONTROL];

  uint8_t MouseButton;
  uint8_t ModifierKey;
  bool IsButtonPressed;

  // the speed in units/second to move
  Vector3 MoveSpeed;

  Vector2 RotationSpeed;

  float MouseSensitivity;

  float ClickTimer;

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

void RTSCameraInit(RTSCamera *camera, float fovY, Vector3 position, TerrainMap *terrainMap);

void RTSCameraUseMouse(RTSCamera *camera, bool useMouse, int button);

Vector3 RTSCameraGetWorldPosition(RTSCamera *camera);

Vector3 RTSCameraGetTerrainPosition(RTSCamera *camera);

void RTSCameraSetPosition(RTSCamera *camera, Vector3 position);

Ray RTSCameraGetViewRay(RTSCamera *camera);

void RTSCameraUpdate(RTSCamera *camera, TerrainMap *terrainMap);

void RTSCameraBeginMode3D(RTSCamera *camera);

void RTSCameraEndMode3D(void);

void RTSCameraClearInputs(RTSCamera *camera);
