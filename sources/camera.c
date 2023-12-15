#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "raylib.h"
#include "rlgl.h"

static void ResizeRTSOrbitCameraView(RTSCamera *camera)
{
  if (!camera)
  {
    return;
  }

  float width = (float)GetScreenWidth();
  float height = (float)GetScreenHeight();

  camera->FOV.y = camera->ViewCamera.fovy;

  if (height != 0)
    camera->FOV.x = camera->FOV.y * (width / height);
}

void RTSCameraInit(RTSCamera *camera, float fovY, Vector3 position, TerrainMap *terrainMap)
{
  if (!camera)
    return;

  camera->ControlsKeys[0] = 'W';
  camera->ControlsKeys[1] = 'S';
  camera->ControlsKeys[2] = 'D';
  camera->ControlsKeys[3] = 'A';
  camera->ControlsKeys[4] = KEY_SPACE;
  camera->ControlsKeys[5] = KEY_LEFT_CONTROL;
  camera->ControlsKeys[6] = 'E';
  camera->ControlsKeys[7] = 'Q';
  camera->ControlsKeys[8] = KEY_UP;
  camera->ControlsKeys[9] = KEY_DOWN;
  camera->ControlsKeys[10] = KEY_LEFT_SHIFT;

  camera->MouseButton = 0;
  camera->ModifierKey = 0;

  camera->ClickTimer = 0;

  camera->MoveSpeed = (Vector3){8, 8, 8};
  camera->RotationSpeed = (Vector2){90, 90};

  camera->MouseSensitivity = 600;

  camera->MinimumViewY = -50.0f;
  camera->MaximumViewY = -15.0f;


  camera->Focused = IsWindowFocused();

  camera->CameraPullbackDistance = 15.f;

  camera->ViewAngles = (Vector2){180.f * DEG2RAD, -45.f * DEG2RAD};

  camera->CameraPosition = position;
  camera->CameraPosition.y = GetAdjustedHeight(camera->CameraPosition, terrainMap);
  camera->FOV.y = fovY;

  Matrix tiltMat = MatrixRotateX(camera->ViewAngles.y);

  camera->ViewCamera.target = position;
  camera->ViewCamera.position = Vector3Add(camera->ViewCamera.target, (Vector3){0, 0, camera->CameraPullbackDistance});
  camera->ViewCamera.position = Vector3Transform(camera->ViewCamera.position, tiltMat);
  camera->ViewCamera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera->ViewCamera.fovy = fovY;
  camera->ViewCamera.projection = CAMERA_PERSPECTIVE;

  camera->NearPlane = 0.01;
  camera->FarPlane = 1000.0;

  ResizeRTSOrbitCameraView(camera);
}

Vector3 RTSCameraGetPosition(RTSCamera *camera)
{
  // will need to harmonize with terrain coordinates
  return camera->CameraPosition;
}

void RTSCameraSetPosition(RTSCamera *camera, Vector3 position)
{
  // uses world coordinates
  camera->CameraPosition = position;
}

Ray RTSCameraGetViewRay(RTSCamera *camera)
{
  return (Ray){camera->ViewCamera.position, Vector3Subtract(camera->ViewCamera.target, camera->ViewCamera.position)};
}

static float GetSpeedForAxis(RTSCamera *camera, RTSCameraControls axis, float speed)
{
  if (!camera)
    return 0;

  int key = camera->ControlsKeys[axis];
  if (key == -1)
    return 0;

  float factor = 1.0f;

  if (IsKeyDown(camera->ControlsKeys[axis]))
    return speed * GetFrameTime() * factor;

  return 0.0f;
}

void RTSCameraUpdate(RTSCamera *camera, TerrainMap *terrainMap)
{
  if (!camera)
    return;

  if (IsWindowResized())
    ResizeRTSOrbitCameraView(camera);
  camera->Focused = IsWindowFocused();

  bool isPressed = false;

  if (camera->Focused)
  {
    for (int button = MOUSE_BUTTON_LEFT; button <= MOUSE_BUTTON_MIDDLE; button++)
    {
      if (IsMouseButtonDown(button))
      {
        camera->MouseButton = button;
        camera->IsButtonPressed = true;
        break;
      }
      else if (IsMouseButtonReleased(button))
      {
        camera->MouseButton = button;
        camera->IsButtonPressed = false;
        break;
      }
    }
    if (IsKeyDown(camera->ControlsKeys[MODIFIER_1]))
    {
      camera->ModifierKey = ADDITIONAL_MODIFIER;
    }
    else
    {
      camera->ModifierKey = 0;
    }
  }
  else
    (camera->IsButtonPressed = false); // no buttons registered when not focused

  Vector2 mousePositionDelta = GetMouseDelta();
  // float mouseWheelMove = GetMouseWheelMove();

  float direction[MOVE_DOWN + 1] = {
      -GetSpeedForAxis(camera, MOVE_FRONT, camera->MoveSpeed.z),
      -GetSpeedForAxis(camera, MOVE_BACK, camera->MoveSpeed.z),
      GetSpeedForAxis(camera, MOVE_RIGHT, camera->MoveSpeed.x),
      GetSpeedForAxis(camera, MOVE_LEFT, camera->MoveSpeed.x),
      GetSpeedForAxis(camera, MOVE_UP, camera->MoveSpeed.y),
      GetSpeedForAxis(camera, MOVE_DOWN, camera->MoveSpeed.y)};

  bool rotateMouse = (camera->MouseButton == MOUSE_BUTTON_MIDDLE && camera->IsButtonPressed);
  if (rotateMouse)
  {
    HideCursor();
  }
  else
  {
    ShowCursor();
  }

  float pivotHeadingRotation =
      GetSpeedForAxis(camera, ROTATE_RIGHT, camera->RotationSpeed.x) -
      GetSpeedForAxis(camera, ROTATE_LEFT, camera->RotationSpeed.x);
  float pivotPitchRotation =
      GetSpeedForAxis(camera, ROTATE_UP, camera->RotationSpeed.y) -
      GetSpeedForAxis(camera, ROTATE_DOWN, camera->RotationSpeed.y);

  if (pivotHeadingRotation)
    camera->ViewAngles.x -= pivotHeadingRotation * DEG2RAD;
  else if (rotateMouse && camera->Focused)
    camera->ViewAngles.x -= (mousePositionDelta.x / camera->MouseSensitivity);

  if (pivotPitchRotation)
    camera->ViewAngles.y += pivotPitchRotation * DEG2RAD;
  else if (rotateMouse && camera->Focused)
    camera->ViewAngles.y += (mousePositionDelta.y / -camera->MouseSensitivity);

  // Clamp view angles
  if (camera->ViewAngles.y < camera->MinimumViewY * DEG2RAD)
    camera->ViewAngles.y = camera->MinimumViewY * DEG2RAD;
  else if (camera->ViewAngles.y > camera->MaximumViewY * DEG2RAD)
    camera->ViewAngles.y = camera->MaximumViewY * DEG2RAD;

  Vector3 moveVec = {0};
  moveVec.x = direction[MOVE_RIGHT] - direction[MOVE_LEFT];
  moveVec.z = direction[MOVE_FRONT] - direction[MOVE_BACK];

  // Update zoom
  camera->CameraPullbackDistance += (-GetMouseWheelMove()) + (direction[MOVE_DOWN] - direction[MOVE_UP]);
  if (camera->CameraPullbackDistance < 1)
    camera->CameraPullbackDistance = 1;

  Vector3 camPos = {.z = camera->CameraPullbackDistance};

  Matrix tiltMat = MatrixRotateX(camera->ViewAngles.y);
  Matrix rotMat = MatrixRotateY(camera->ViewAngles.x);
  Matrix mat = MatrixMultiply(tiltMat, rotMat);

  camPos = Vector3Transform(camPos, mat);
  moveVec = Vector3Transform(moveVec, rotMat);

  camera->CameraPosition = Vector3Add(camera->CameraPosition, moveVec);
  camera->CameraPosition.y = GetAdjustedHeight(camera->CameraPosition, terrainMap);
  camera->ViewCamera.target = camera->CameraPosition;
  camera->ViewCamera.position = Vector3Add(camera->CameraPosition, camPos);

  camera->ClickTimer -= GetFrameTime();
}

static void SetupCamera(RTSCamera *camera, float aspect)
{
  rlDrawRenderBatchActive();
  rlMatrixMode(RL_PROJECTION);
  rlPushMatrix();
  rlLoadIdentity();

  if (camera->ViewCamera.projection == CAMERA_PERSPECTIVE)
  {
    double top =
        RL_CULL_DISTANCE_NEAR * tan(camera->ViewCamera.fovy * 0.5 * DEG2RAD);
    double right = top * aspect;

    rlFrustum(-right, right, -top, top, camera->NearPlane, camera->FarPlane);
  }
  else if (camera->ViewCamera.projection == CAMERA_ORTHOGRAPHIC)
  {
    double top = camera->ViewCamera.fovy / 2.0;
    double right = top * aspect;

    rlOrtho(-right, right, -top, top, camera->NearPlane, camera->FarPlane);
  }

  rlMatrixMode(RL_MODELVIEW);
  rlLoadIdentity();

  Matrix matView =
      MatrixLookAt(camera->ViewCamera.position, camera->ViewCamera.target,
                   camera->ViewCamera.up);

  rlMultMatrixf(MatrixToFloatV(matView).v);

  rlEnableDepthTest();
}

void RTSCameraBeginMode3D(RTSCamera *camera)
{
  if (!camera)
    return;

  float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
  SetupCamera(camera, aspect);
}

void RTSCameraEndMode3D(void)
{
  EndMode3D();
}
