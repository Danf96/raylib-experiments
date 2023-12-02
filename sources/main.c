#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "camera.h"
#include "scene.h"
#include "model.h"

#define screenWidth 1280
#define screenHeight 720

typedef struct MeshList
{
  size_t capacity;
  size_t size;
  Mesh *mesh;
} MeshList;

typedef struct MaterialList
{
  size_t capacity;
  size_t size;
  Material *mat;
} MaterialList;

Vector2 terrainOffset = {};
HeightMap heightMap;

int main(void)
{
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
  heightMap = (HeightMap){.width = discMap.width, .height = discMap.height};
  heightMap.value = MemAlloc(sizeof(*heightMap.value) * discMap.width * discMap.height);
  if (!heightMap.value)
  {
    TraceLog(LOG_ERROR, "Failed to allocate heightmap memory.");
    return EXIT_FAILURE;
  }
  Mesh terrainMesh = GenMeshCustomHeightmap(discMap, &heightMap);
  Material terrainMaterial = LoadMaterialDefault();
  terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = colorMap;
  terrainOffset = (Vector2){.x = heightMap.width / 2.0f, .y = heightMap.height / 2.0f};

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
  Matrix terrMatrix = MatrixTranslate(-terrainOffset.x, 0.0f, -terrainOffset.y);

  // Entity List

  EntityList entityList = CreateEntityList(5);
  EntityCreate newEnt = (EntityCreate){.scale = (Vector3){1.0f, 1.0f, 1.0f},
                                       .position = (Vector3){.x = terrainOffset.x, .y = 0.5f, .z = terrainOffset.y},
                                       .materialHandle = 0,
                                       .typeHandle = 1,
                                       .type = NODE_TYPE_MODEL};
  if (AddEntity(&entityList, &newEnt))
  {
    return EXIT_FAILURE;
  }

  // SetTargetFPS(60);


  RTSCamera camera;
  RTSCameraInit(&camera, 45.0f, (Vector3){0, 0, 0});
  camera.ViewAngles.y = -15 * DEG2RAD;


  float simTime = 0;
  float prevSimTime = 0;
  float simDt = 1.f / 60.f;

  //--------------------------------------------------------------------------
  // Main game loop
  //--------------------------------------------------------------------------
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    //----------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------
    simTime = GetTime();
    for (; simTime >= prevSimTime + simDt;)
    {
      const float MOVE_SPEED = 0.25f;
      // spriteRoot->rotation = QuaternionMultiply(spriteRoot->rotation, fixedRotation);

      entityList.entities[0].rotation.y += 0.05f;
      entityList.entities[0].isDirty = true;
      
      TraceLog(LOG_INFO, TextFormat("%f", camera.CameraPosition.y));
      
      // camera.target = Player->position;
      RTSCameraUpdate(&camera);
      RTSCameraAdjustHeight(&camera, &heightMap);
      prevSimTime += simDt;
    }
    UpdateDirtyEntities(&entityList, &heightMap);

    //----------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    RTSCameraBeginMode3D(&camera);

    // Draw Terrain
    DrawMesh(meshList.mesh[0], matList.mat[0], terrMatrix);

    for (size_t i = 0; i < entityList.size; i++)
    {
      Entity ent = entityList.entities[i];
      if (ent.type == NODE_TYPE_MODEL)
      {
        DrawMesh(meshList.mesh[ent.typeHandle], matList.mat[ent.materialHandle], ent.worldMatrix);
      }
    }

    DrawSphere(camera.CameraPosition, 0.25f, RED);

    BeginShaderMode(alphaDiscard);
#if 0

    for (auto &job : billBatch) {
      Vector3 newPos = getAdjustedPosition(job.position, heightLevels);
      DrawBillboard(camera, *billboardList[job.textureID], newPos, 1.0f,
                    WHITE);
    }
#endif

    EndShaderMode();

    // red outer circle
    // DrawCircle3D(Vector3Zero(), 2, (Vector3){1, 0, 0}, 90, RED);

    // skybox, to be drawn last
    DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, GREEN);
    
    RTSCameraEndMode3D();
    DrawFPS(10, 10);
    DrawText(TextFormat("%.4f\n%.4f\n%05.4f", camera.CameraPosition.x + terrainOffset.x, camera.CameraPosition.y, camera.CameraPosition.z + terrainOffset.y), 10, 30, 20, WHITE);

    EndDrawing();
  }

  //--------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------

  UnloadShader(skybox.materials[0].shader);
  UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
  UnloadModel(skybox);

  for (size_t i = 0; i < meshList.size; i++)
  {
    UnloadMesh(meshList.mesh[i]);
  }
  for (size_t i = 0; i < matList.size; i++)
  {
    UnloadMaterial(matList.mat[i]);
  }

  UnloadTexture(terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);
  // Free entities here
  MemFree(entityList.entities);

  UnloadTexture(bill);

  CloseWindow();

  return 0;
}
