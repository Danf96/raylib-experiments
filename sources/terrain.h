#pragma once
#include <stdlib.h>
#include "raylib.h"
#include "raymath.h"
#include "stddef.h"

// intended for all terrain code, everything outside this module should rely on
// world coordinates

typedef struct
{
  int maxWidth;
  int maxHeight;
  float *value; // treat as 2d array, may convert to struct later
} TerrainMap;

// modifies X and Z world coordinates to match indices of terrain map lookups
Vector3 WorldXZToTerrain(Vector3 worldPos, TerrainMap *terrainMap);

Vector3 TerrainXZToWorld(Vector3 terrainPos, TerrainMap *terrainMap);

// takes in world coordinates and gives an approximation of the height using barycentric coordinates
float GetAdjustedHeight(Vector3 worldPos, TerrainMap *terrainMap);

// raylib mesh generation, while also populating an array of floats for future lookups
Mesh GenMeshCustomHeightmap(Image heightImage, TerrainMap *terrainMap);

Vector3 GetRayPointTerrain(Ray ray, TerrainMap *terrainMap, float zNear, float zFar);