#include "skybox.h"
#include <stdio.h>

Model GetSkybox(const char *skyName)
{
  // side endings in order they will be loaded onto in the cubemap (currently reversed)
  char *sides[] = {"E.png", "W.png", "T.png", "B.png", "S.png", "N.png"};
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

  // Skybox generation, assumes skybox names are in the order of East, West, Top, Bottom, South, North
  char skySides[6][32] = {};
  Image img[6];
  for (int i = 0; i < 6; i++) 
  {
    sprintf(skySides[i], "%s%s", skyName, sides[i]);
    img[i] = LoadImage(skySides[i]);
    ImageFlipHorizontal(&img[i]);
    if (i == 2 || i == 3)
    {
      ImageFlipVertical(&img[i]);
    }
    // TODO: if null, load a default
  }
  
   
  Image faces = {0}; // Vertical column image
  int size = img[0].width;
  faces = GenImageColor(size, size * 6, MAGENTA);
  ImageFormat(&faces, img[0].format); // currently assuming all images are same size and format
  for (int i = 0; i < 6; i++)
  {
    Rectangle srcRec = (Rectangle){0, 0, size, size};
    Rectangle dstRec = (Rectangle){0, (float)size * (float)i, size, size};
    
    ImageDraw(&faces, img[i], srcRec, dstRec, WHITE);
  }
  skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(
      faces, CUBEMAP_LAYOUT_AUTO_DETECT); // CUBEMAP_LAYOUT_PANORAMA
      for (int i = 0; i < 6; i++) 
  {
    UnloadImage(img[i]);
  }
  
  UnloadImage(faces);
  return skybox;
}

