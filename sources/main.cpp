#include <stddef.h>
#include <vector>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "scene.hpp"

#define screenWidth 1280
#define screenHeight 720

struct HeightMap {
  int width;
  int height;
  std::vector<float> vertices;
  std::vector<unsigned int> indices;
};

Model getSkybox(const char *skyName);

Mesh GenMeshCustomHeightmap(Image heightmap, std::vector<std::vector<float>> &heightLevels);

constexpr float getGrayVal(Color c) {
  return ((float)(c.r + c.g + c.b) / 3.0f);
}

int main(void) {
  //--------------------------------------------------------------------------
  // Initialization
  //--------------------------------------------------------------------------
  InitWindow(screenWidth, screenHeight, "raylib - demo");

  // Define the camera to look into our 3d world
  Camera camera = {0};
  camera.position = (Vector3){0.0f, 0.5f, 0.0f};
  camera.target = (Vector3){0.0f, 0.0f, 1.0f};
  camera.up = (Vector3){0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;

  // textures, models, and shaders
  Texture2D bill = LoadTexture("../resources/billboard.png");
  Shader alphaDiscard = LoadShader(NULL, "../shaders/alphaDiscard.fs");

  GenTextureMipmaps(&bill);
  SetTextureWrap(bill, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(bill, TEXTURE_FILTER_ANISOTROPIC_16X);

  Model skybox = getSkybox("../resources/agent.png");

  // terrain
  Image discMap = LoadImage("../resources/discmap.BMP");
  Texture2D colorMap = LoadTexture("../resources/colormap.BMP");
  std::vector<std::vector<float>> heightLevels;
  heightLevels.resize(discMap.width, std::vector<float>(discMap.height));
  Mesh terrainMesh = GenMeshCustomHeightmap(discMap, heightLevels);
  Material terrainMaterial = LoadMaterialDefault();
  terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture = colorMap;

  GenTextureMipmaps(&colorMap);
  SetTextureWrap(colorMap, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(colorMap, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(colorMap, TEXTURE_FILTER_ANISOTROPIC_16X);

  //note, need a new data structure for storing heightmap info for quick reference
  int mapWidth = discMap.width;
  int mapHeight = discMap.height;

  UnloadImage(discMap);

  // cube for testing
  Mesh cube = GenMeshCube(1, 1, 1);

  std::vector<Mesh *> meshList;
  std::vector<Material *> materialList;
  std::vector<Texture2D *> billboardList;

  meshList.push_back(&terrainMesh);
  materialList.push_back(&terrainMaterial);
  meshList.push_back(&cube);
  billboardList.push_back(&bill);

  Scene myScene = Scene();
  myScene.root.createChildNode({(-mapWidth/2.0f), 0.0f, (-mapHeight/2.0f)}, QuaternionIdentity(), 0, 0,
                               NODE_TYPE_MODEL);
  auto terrainRoot = &myScene.root.children[0];
  terrainRoot->createChildNode({mapWidth/2.0f, 30.0f, (mapHeight/2.0f)}, QuaternionIdentity(),
                                           1, 0, NODE_TYPE_EMPTY);
  auto spriteRoot = &terrainRoot->children[0];
  spriteRoot->createChildNode(
      {1, 0, 0}, QuaternionIdentity(), 0, 0, NODE_TYPE_BILLBOARD);
  spriteRoot->createChildNode(
      {-1, 0, 0}, QuaternionIdentity(), 0, 0, NODE_TYPE_BILLBOARD);
  spriteRoot->createChildNode(
      {0, 0, 1}, QuaternionIdentity(), 0, 0, NODE_TYPE_BILLBOARD);
  spriteRoot->createChildNode(
      {0, 0, -1}, QuaternionIdentity(), 0, 0, NODE_TYPE_BILLBOARD);

  

  // Vector3 ang = {0}; // model rotation
  Quaternion rotation = QuaternionFromEuler(0, 0.05f, 0);

  SetTargetFPS(60);

  //--------------------------------------------------------------------------
  // Main game loop
  //--------------------------------------------------------------------------
  while (!WindowShouldClose()) // Detect window close button or ESC key
  {
    //----------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------
    // ang = Vector3Add(ang, (Vector3){0.01, 0.005, 0.0025});
    // spriteRoot->position = Vector3Add(spriteRoot->position, {0, 0.0001, 0});
    spriteRoot->rotation = QuaternionMultiply(spriteRoot->rotation, rotation);
    // model.transform = MatrixRotateXYZ(ang);
    UpdateCamera(&camera, CAMERA_PERSPECTIVE);
    std::vector<InstanceModel> modelBatch;
    std::vector<InstanceBill> billBatch;
    myScene.generateRenderBatch(modelBatch, billBatch);

    //----------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    BeginMode3D(camera);

    for (auto &job : modelBatch) {
      DrawMesh(*meshList[job.meshID], *materialList[job.materialID],
               job.finalMatrix);
    }

    // DrawMesh(terrainMesh, terrainMaterial, MatrixIdentity());

    DrawGrid(10, 1.0f);

    // blue inner circle
    DrawCircle3D(Vector3Zero(), 1, (Vector3){1, 0, 0}, 90, BLUE);

    // if space key, isn't held down use the shader
    // space key shows default behaviour
    BeginShaderMode(alphaDiscard);

    for (auto &job : billBatch) {
      DrawBillboard(camera, *billboardList[job.textureID], job.position, 1.0f,
                    WHITE);
    }

    EndShaderMode();

    // red outer circle
    DrawCircle3D(Vector3Zero(), 2, (Vector3){1, 0, 0}, 90, RED);

    // skybox, to be drawn last
    DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, GREEN);

    EndMode3D();
    DrawFPS(10, 10);

    EndDrawing();
  }

  //--------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------

  UnloadShader(skybox.materials[0].shader);
  UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
  UnloadModel(skybox);
  for (auto mesh : meshList) {
    UnloadMesh(*mesh);
  }
  for (auto material : materialList) {
    UnloadMaterial(*material);
  }
  UnloadTexture(terrainMaterial.maps[MATERIAL_MAP_DIFFUSE].texture);
  // UnloadMaterial(terrainMaterial);
  // UnloadMesh(terrainMesh);
  UnloadTexture(bill);

  CloseWindow();

  return 0;
}

Model getSkybox(const char *skyName) {
  // a 3D model
  Mesh cube = GenMeshCube(-1.0f, -1.0f, -1.0f);

  Model skybox = LoadModelFromMesh(cube);
  skybox.materials[0].shader =
      LoadShader("../shaders/skybox.vs", "../shaders/skybox.fs");
  {
    int arr[1] = {MATERIAL_MAP_CUBEMAP};
    SetShaderValue(
        skybox.materials[0].shader,
        GetShaderLocation(skybox.materials[0].shader, "environmentMap"), arr,
        SHADER_UNIFORM_INT);
    arr[0] = 0;
    SetShaderValue(skybox.materials[0].shader,
                   GetShaderLocation(skybox.materials[0].shader, "doGamma"),
                   arr, SHADER_UNIFORM_INT);
  }

  // Skybox generation, refactor later to take variations of input string_front,
  // _back, etc
  Image img = LoadImage(skyName);
  Image faces = {0}; // Vertical column image
  int size = img.width;
  faces = GenImageColor(size, size * 6, MAGENTA);
  ImageFormat(&faces, img.format);
  for (int i = 0; i < 6; i++) {
    Rectangle srcRec{0, 0, static_cast<float>(size), static_cast<float>(size)};
    Rectangle dstRec{0, static_cast<float>(size) * static_cast<float>(i),
                     static_cast<float>(size), static_cast<float>(size)};
    ImageDraw(&faces, img, srcRec, dstRec, WHITE);
  }
  skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(
      faces, CUBEMAP_LAYOUT_AUTO_DETECT); // CUBEMAP_LAYOUT_PANORAMA
  UnloadImage(img);
  UnloadImage(faces);
  return skybox;
}

// Modified version of RayLib heighmap generation, will be refactored to store heightmap data outside of GPU
Mesh GenMeshCustomHeightmap(Image heightmap, std::vector<std::vector<float>> &heightLevels) {

  Mesh mesh = {0};

  int mapX = heightmap.width;
  int mapZ = heightmap.height;

  float yScale = 64.0f / 256.0f;

  Color *pixels = LoadImageColors(heightmap);

  // NOTE: One vertex per pixel
  mesh.triangleCount =
      (mapX - 1) * (mapZ - 1) * 2; // One quad every four pixels

  mesh.vertexCount = mesh.triangleCount * 3;

  mesh.vertices = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals = (float *)RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.texcoords = (float *)RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
  mesh.colors = NULL;

  int vCounter = 0;  // Used to count vertices float by float
  int tcCounter = 0; // Used to count texcoords float by float
  int nCounter = 0;  // Used to count normals float by float

  Vector3 vA = {0};
  Vector3 vB = {0};
  Vector3 vC = {0};
  Vector3 vN = {0};

  for (int z = 0; z < mapZ - 1; z++) {
    for (int x = 0; x < mapX - 1; x++) {
      // Fill vertices array with data
      //----------------------------------------------------------

      // one triangle - 3 vertex
      mesh.vertices[vCounter] = (float)x;
      mesh.vertices[vCounter + 1] = getGrayVal(pixels[x + z * mapX]) * yScale;
      mesh.vertices[vCounter + 2] = (float)z;

      mesh.vertices[vCounter + 3] = (float)x;
      mesh.vertices[vCounter + 4] =
          getGrayVal(pixels[x + (z + 1) * mapX]) * yScale;
      mesh.vertices[vCounter + 5] = (float)(z + 1);

      mesh.vertices[vCounter + 6] = (float)(x + 1);
      mesh.vertices[vCounter + 7] =
          getGrayVal(pixels[(x + 1) + z * mapX]) * yScale;
      mesh.vertices[vCounter + 8] = (float)z;

      // populate heightMap
      heightLevels[x][z] = getGrayVal(pixels[x + z * mapX]) * yScale;
      heightLevels[x][z + 1] = getGrayVal(pixels[x + (z + 1) * mapX]) * yScale;
      heightLevels[x + 1][z] = getGrayVal(pixels[(x + 1) + z * mapX]) * yScale;
      heightLevels[x + 1][z + 1] = getGrayVal(pixels[(x + 1) + (z + 1) * mapX]) * yScale;

      // Another triangle - 3 vertex
      mesh.vertices[vCounter + 9] = mesh.vertices[vCounter + 6];
      mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
      mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

      mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
      mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
      mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

      mesh.vertices[vCounter + 15] = (float)(x + 1);
      mesh.vertices[vCounter + 16] =
          getGrayVal(pixels[(x + 1) + (z + 1) * mapX]) * yScale;
      mesh.vertices[vCounter + 17] = (float)(z + 1);
      vCounter += 18; // 6 vertex, 18 floats

      // Fill texcoords array with data
      //--------------------------------------------------------------
      mesh.texcoords[tcCounter] = (float)x / (mapX - 1);
      mesh.texcoords[tcCounter + 1] = (float)z / (mapZ - 1);

      mesh.texcoords[tcCounter + 2] = (float)x / (mapX - 1);
      mesh.texcoords[tcCounter + 3] = (float)(z + 1) / (mapZ - 1);

      mesh.texcoords[tcCounter + 4] = (float)(x + 1) / (mapX - 1);
      mesh.texcoords[tcCounter + 5] = (float)z / (mapZ - 1);

      mesh.texcoords[tcCounter + 6] = mesh.texcoords[tcCounter + 4];
      mesh.texcoords[tcCounter + 7] = mesh.texcoords[tcCounter + 5];

      mesh.texcoords[tcCounter + 8] = mesh.texcoords[tcCounter + 2];
      mesh.texcoords[tcCounter + 9] = mesh.texcoords[tcCounter + 3];

      mesh.texcoords[tcCounter + 10] = (float)(x + 1) / (mapX - 1);
      mesh.texcoords[tcCounter + 11] = (float)(z + 1) / (mapZ - 1);
      tcCounter += 12; // 6 texcoords, 12 floats

      // Fill normals array with data
      //--------------------------------------------------------------
      for (int i = 0; i < 18; i += 9) {
        vA.x = mesh.vertices[nCounter + i];
        vA.y = mesh.vertices[nCounter + i + 1];
        vA.z = mesh.vertices[nCounter + i + 2];

        vB.x = mesh.vertices[nCounter + i + 3];
        vB.y = mesh.vertices[nCounter + i + 4];
        vB.z = mesh.vertices[nCounter + i + 5];

        vC.x = mesh.vertices[nCounter + i + 6];
        vC.y = mesh.vertices[nCounter + i + 7];
        vC.z = mesh.vertices[nCounter + i + 8];

        vN = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(vB, vA),
                                                  Vector3Subtract(vC, vA)));

        mesh.normals[nCounter + i] = vN.x;
        mesh.normals[nCounter + i + 1] = vN.y;
        mesh.normals[nCounter + i + 2] = vN.z;

        mesh.normals[nCounter + i + 3] = vN.x;
        mesh.normals[nCounter + i + 4] = vN.y;
        mesh.normals[nCounter + i + 5] = vN.z;

        mesh.normals[nCounter + i + 6] = vN.x;
        mesh.normals[nCounter + i + 7] = vN.y;
        mesh.normals[nCounter + i + 8] = vN.z;
      }

      nCounter += 18; // 6 vertex, 18 floats
    }
  }

  UnloadImageColors(pixels); // Unload pixels color data

  // Upload vertex data to GPU (static mesh)
  UploadMesh(&mesh, false);

  return mesh;
}
