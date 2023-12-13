#include "terrain.h"
// Modified version of RayLib heighmap generation
Mesh GenMeshCustomHeightmap(Image heightImage, TerrainMap *terrainMap)
{
#define GRAY_VALUE(c) ((float)(c.r + c.g + c.b) / 3.0f)

  Mesh mesh = {0};

  int mapX = heightImage.width;
  int mapZ = heightImage.height;

  const float yScale = 32.0f / 256.0f;

  Color *pixels = LoadImageColors(heightImage);

  // NOTE: One vertex per pixel
  mesh.triangleCount =
      (mapX - 1) * (mapZ - 1) * 2; // One quad every four pixels

  mesh.vertexCount = mesh.triangleCount * 3;

  mesh.vertices = RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.normals = RL_MALLOC(mesh.vertexCount * 3 * sizeof(float));
  mesh.texcoords = RL_MALLOC(mesh.vertexCount * 2 * sizeof(float));
  mesh.colors = NULL;

  int vCounter = 0;  // Used to count vertices float by float
  int tcCounter = 0; // Used to count texcoords float by float
  int nCounter = 0;  // Used to count normals float by float

  Vector3 vA = {0};
  Vector3 vB = {0};
  Vector3 vC = {0};
  Vector3 vN = {0};

  for (int z = 0; z < mapZ - 1; z++)
  {
    for (int x = 0; x < mapX - 1; x++)
    {
      // Fill vertices array with data
      //----------------------------------------------------------

      // one triangle - 3 vertex
      mesh.vertices[vCounter] = (float)x;
      mesh.vertices[vCounter + 1] = GRAY_VALUE(pixels[x + z * mapX]) * yScale;
      mesh.vertices[vCounter + 2] = (float)z;

      mesh.vertices[vCounter + 3] = (float)x;
      mesh.vertices[vCounter + 4] =
          GRAY_VALUE(pixels[x + (z + 1) * mapX]) * yScale;
      mesh.vertices[vCounter + 5] = (float)(z + 1);

      mesh.vertices[vCounter + 6] = (float)(x + 1);
      mesh.vertices[vCounter + 7] =
          GRAY_VALUE(pixels[(x + 1) + z * mapX]) * yScale;
      mesh.vertices[vCounter + 8] = (float)z;

      // populate heightMap
      terrainMap->value[x * terrainMap->maxWidth + z] = GRAY_VALUE(pixels[x + z * mapX]) * yScale;
      terrainMap->value[x * terrainMap->maxWidth + (z + 1)] = GRAY_VALUE(pixels[x + (z + 1) * mapX]) * yScale;
      terrainMap->value[(x + 1) * terrainMap->maxWidth + z] = GRAY_VALUE(pixels[(x + 1) + z * mapX]) * yScale;
      terrainMap->value[(x + 1) * terrainMap->maxWidth + (z + 1)] = GRAY_VALUE(pixels[(x + 1) + (z + 1) * mapX]) * yScale;

      // Another triangle - 3 vertex
      mesh.vertices[vCounter + 9] = mesh.vertices[vCounter + 6];
      mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
      mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

      mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
      mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
      mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

      mesh.vertices[vCounter + 15] = (float)(x + 1);
      mesh.vertices[vCounter + 16] = GRAY_VALUE(pixels[(x + 1) + (z + 1) * mapX]) * yScale;
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
      for (int i = 0; i < 18; i += 9)
      {
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

Vector3 WorldXZToTerrain(Vector3 worldPos, TerrainMap *terrainMap)
{
  return (Vector3){.x = worldPos.x + (terrainMap->maxWidth / 2.0f),
                   .y = worldPos.y,
                   .z = worldPos.z + (terrainMap->maxHeight / 2.0f)};
}

Vector3 TerrainXZToWorld(Vector3 terrainPos, TerrainMap *terrainMap)
{
  return (Vector3){.x = terrainPos.x - (terrainMap->maxWidth / 2.0f),
                   .y = terrainPos.y,
                   .z = terrainPos.z - (terrainMap->maxHeight / 2.0f)};
}

float GetAdjustedHeight(Vector3 worldPos, TerrainMap *terrainMap)
{
  Vector3 terrainPos = WorldXZToTerrain(worldPos, terrainMap);
  int indexX = floor(terrainPos.x);
  int indexZ = floor(terrainPos.z);
  if (indexX >= terrainMap->maxWidth || indexZ >= terrainMap->maxHeight)
  {
    // we are out of bounds
    return 0.0f;
  }
  float xCoord = (terrainPos.x - indexX);
  float zCoord = (terrainPos.z - indexZ);
  Vector3 a, b, c;    // three vectors constructed around oldPos
  Vector3 barycenter; // u, v, w calculated from a, b, c
  float answer;
  if (xCoord <= 1 - zCoord)
  {
    a = (Vector3){0, terrainMap->value[indexX * terrainMap->maxWidth + indexZ], 0}; // maxWidth used as a stride offset
    b = (Vector3){1, terrainMap->value[(indexX + 1) * terrainMap->maxWidth + indexZ], 0};
    c = (Vector3){0, terrainMap->value[indexX * terrainMap->maxWidth + (indexZ + 1)], 1};
  }
  else
  {
    a = (Vector3){1, terrainMap->value[(indexX + 1) * terrainMap->maxWidth + (indexZ )], 0};
    b = (Vector3){1, terrainMap->value[(indexX + 1) * terrainMap->maxWidth + (indexZ + 1)], 1};
    c = (Vector3){0, terrainMap->value[(indexX) * terrainMap->maxWidth + (indexZ + 1)], 1};
  }
  barycenter = Vector3Barycenter((Vector3){xCoord, terrainPos.y, zCoord}, a, b, c);
  answer = barycenter.x * a.y + barycenter.y * b.y + barycenter.z * c.y;

  return answer;
}

// Get collision info between ray and terrain
// sourced from  https://iquilezles.org/articles/terrainmarching/
Vector3 GetRayPointTerrain(Ray ray, TerrainMap *terrainMap, float zNear, float zFar)
{
  // calculations will be done using terrain coordinates until testing collision
  Vector3 rayPos = WorldXZToTerrain(ray.position, terrainMap);

  // change in direction in 3 axes
  float tDelta = 0.5f;
  Vector3 p;
  bool intersection = false;
  for (float t = zNear; t < zFar; t += tDelta)
  {
    p = Vector3Add(rayPos, Vector3Scale(ray.direction, t));
    // check to ensure no segfaults
    if (p.x < 0 || p.z < 0 || p.x > terrainMap->maxWidth - 1 || p.z > terrainMap->maxHeight - 1) {
      return (Vector3){0};
    }
    float heightY = terrainMap->value[(int)floor(p.x) * terrainMap->maxWidth + (int)floor(p.z)];
    if (p.y < heightY)
    {
      // NOTE: does redundant calulcations currently, could move to a new function
      p.y = GetAdjustedHeight(p, terrainMap);
      return TerrainXZToWorld(p, terrainMap);
    }
  }
  return (Vector3){0};
}