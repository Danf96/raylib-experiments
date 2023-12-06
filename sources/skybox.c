#include "skybox.h"

Model GetSkybox(const char *skyName)
{
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

  // Skybox generation, refactor later to take variations of input
  // string_front, _back, etc
  Image img = LoadImage(skyName);
  Image faces = {0}; // Vertical column image
  int size = img.width;
  faces = GenImageColor(size, size * 6, MAGENTA);
  ImageFormat(&faces, img.format);
  for (int i = 0; i < 6; i++)
  {
    Rectangle srcRec = (Rectangle){0, 0, size, size};
    Rectangle dstRec = (Rectangle){0, (float)size * (float)i, size, size};
    ImageDraw(&faces, img, srcRec, dstRec, WHITE);
  }
  skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(
      faces, CUBEMAP_LAYOUT_AUTO_DETECT); // CUBEMAP_LAYOUT_PANORAMA
  UnloadImage(img);
  UnloadImage(faces);
  return skybox;
}