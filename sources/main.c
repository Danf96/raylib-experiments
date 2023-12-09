#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "camera.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "scene.h"
#include "skybox.h"
#include "terrain.h"

#define screenWidth 1280
#define screenHeight 720

typedef struct {
  size_t capacity;
  size_t size;
  Mesh* mesh;
} MeshList;

typedef struct {
  size_t capacity;
  size_t size;
  Material* mat;
} MaterialList;

typedef struct {
  short id;
  BoundingBox dimensions;
} BBox;

Vector2 terrainOffset = {0};
TerrainMap terrainMap = {0};

int main(void) {
  //--------------------------------------------------------------------------
  // Initialization
  //--------------------------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "raylib - demo");

  // textures, models, and shaders
  Texture2D bill = LoadTexture("../resources/billboard.png");
  Shader alphaDiscard = LoadShader(NULL, "../shaders/alphaDiscard.fs");

  GenTextureMipmaps(&bill);
  SetTextureWrap(bill, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(bill, TEXTURE_FILTER_ANISOTROPIC_16X);

  Model skybox = GetSkybox("../resources/agent.png");

  // terrain
  Image discMap = LoadImage("../resources/discmap.BMP");
  Texture2D colorMap = LoadTexture("../resources/colormap.BMP");
  terrainMap = (TerrainMap){.maxWidth = discMap.width, .maxHeight = discMap.height};
  terrainMap.value = MemAlloc(sizeof(*terrainMap.value) * discMap.width * discMap.height);
  if (!terrainMap.value)
  {
    TraceLog(LOG_ERROR, "Failed to allocate heightmap memory.");
    return EXIT_FAILURE;
  }
  Mesh terrainMesh = GenMeshCustomHeightmap(discMap, &terrainMap);
  Material terrainMaterial = LoadMaterialDefault();
  terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = colorMap;
  terrainOffset = (Vector2){.x = terrainMap.maxWidth / 2.0f, .y = terrainMap.maxHeight / 2.0f};

  GenTextureMipmaps(&colorMap);
  SetTextureWrap(colorMap, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(colorMap, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(colorMap, TEXTURE_FILTER_ANISOTROPIC_16X);

  UnloadImage(discMap);

  // cube for testing
  Mesh cube = GenMeshCube(1, 1, 1);

  MeshList meshList = {.capacity = 10, .size = 2};
  meshList.mesh = MemAlloc(sizeof(*meshList.mesh) * 10);
  meshList.mesh[0] = terrainMesh;
  meshList.mesh[1] = cube;

  MaterialList matList = {.capacity = 10, .size = 1};
  matList.mat = MemAlloc(sizeof(*matList.mat) * 10);
  matList.mat[0] = terrainMaterial;

  // Terrain Matrix;
  Matrix terrMatrix = MatrixTranslate(-(terrainMap.maxWidth / 2.0f), 0.0f, -(terrainMap.maxHeight / 2.0f));

  // Entity List

  EntityList entityList = CreateEntityList(5);
  EntityCreate newEnt = (EntityCreate){.scale = (Vector3){1.0f, 1.0f, 1.0f},
                     .position = (Vector3){.x = 0, .y = 0.5f, .z = 0},
                     .dimensions = (Vector3){1, 1, 1},
                     .materialHandle = 0,
                     .typeHandle = 1,
                     .type = ENT_TYPE_ACTOR};
  if (AddEntity(&entityList, &newEnt)) {
    return EXIT_FAILURE;
  }

  SetTargetFPS(200);

  RTSCamera camera;
  RTSCameraInit(&camera, 45.0f, (Vector3){0, 0, 0});
  camera.ViewAngles.y = -15 * DEG2RAD;

  float simAccumulator = 0;
  float simDt = 1.f / 30.f; // how many times a second calculations should be made

  //--------------------------------------------------------------------------
  // Main game loop
  //--------------------------------------------------------------------------
  while (!WindowShouldClose()) {
    // Detect window close button or ESC key
    //----------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------
    RTSCameraUpdate(&camera, &terrainMap);
    simAccumulator += GetFrameTime();
    while (simAccumulator >= simDt) {
      UpdateEntities(&entityList, &terrainMap, &camera);
      simAccumulator -= simDt;
    }

    //----------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    RTSCameraBeginMode3D(&camera);

    // Draw Terrain
    DrawMesh(meshList.mesh[0], matList.mat[0], terrMatrix);


    for (size_t i = 0; i < entityList.size; i++) {
      Entity ent = entityList.entities[i];
      if (ent.type == ENT_TYPE_ACTOR) {
        DrawMesh(meshList.mesh[ent.typeHandle], matList.mat[ent.materialHandle],
                 ent.worldMatrix);
      }
    }

    DrawSphere(camera.CameraPosition, 0.25f, RED);

    BeginShaderMode(alphaDiscard);

    EndShaderMode();

    // red outer circle
    // DrawCircle3D(Vector3Zero(), 2, (Vector3){1, 0, 0}, 90, RED);

    // skybox, to be drawn last
    DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, GREEN);

    RTSCameraEndMode3D();

    DrawFPS(10, 10);
    DrawText(TextFormat("%.4f\n%.4f\n%05.4f",
                        camera.CameraPosition.x + terrainOffset.x,
                        camera.CameraPosition.y,
                        camera.CameraPosition.z + terrainOffset.y),
             10, 30, 20, WHITE);

    EndDrawing();
  }

  //--------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------

  UnloadShader(skybox.materials[0].shader);
  UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
  UnloadModel(skybox);

  for (size_t i = 0; i < meshList.size; i++) {
    UnloadMesh(meshList.mesh[i]);
  }
  for (size_t i = 0; i < matList.size; i++) {
    UnloadMaterial(matList.mat[i]);
  }

  UnloadTexture(terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);
  // Free entities here
  MemFree(entityList.entities);

  UnloadTexture(bill);

  CloseWindow();

  return 0;
}
