#pragma once
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "stddef.h"

// intended for all new code involved with vertices, meshes, models

typedef struct HeightMap
{
  int width;
  int height;
  float *value; // treat as 2d array
} HeightMap;

float GetAdjustedPosition(Vector3 oldPos, HeightMap *heightMap);

Model GetSkybox(const char *skyName);

Mesh GenMeshCustomHeightmap(Image heightImage, HeightMap *heightMap);