#include "model.h"
// Modified version of RayLib heighmap generation
Mesh GenMeshCustomHeightmap(Image heightImage, HeightMap *heightMap) {
  
  #define GRAY_VALUE(c) ((float)(c.r + c.g + c.b)/3.0f)

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

  for (int z = 0; z < mapZ - 1; z++) {
    for (int x = 0; x < mapX - 1; x++) {
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
      heightMap->value[x * heightMap->width + z] = GRAY_VALUE(pixels[x + z * mapX]) * yScale;
      heightMap->value[x * heightMap->width + (z + 1)] = GRAY_VALUE(pixels[x + (z + 1) * mapX]) * yScale;
      heightMap->value[(x + 1) * heightMap->width + z] = GRAY_VALUE(pixels[(x + 1) + z * mapX]) * yScale;
      heightMap->value[(x + 1) * heightMap->width + (z + 1)] = GRAY_VALUE(pixels[(x + 1) + (z + 1) * mapX]) * yScale;

      // Another triangle - 3 vertex
      mesh.vertices[vCounter + 9] = mesh.vertices[vCounter + 6];
      mesh.vertices[vCounter + 10] = mesh.vertices[vCounter + 7];
      mesh.vertices[vCounter + 11] = mesh.vertices[vCounter + 8];

      mesh.vertices[vCounter + 12] = mesh.vertices[vCounter + 3];
      mesh.vertices[vCounter + 13] = mesh.vertices[vCounter + 4];
      mesh.vertices[vCounter + 14] = mesh.vertices[vCounter + 5];

      mesh.vertices[vCounter + 15] = (float)(x + 1);
      mesh.vertices[vCounter + 16] =
          GRAY_VALUE(pixels[(x + 1) + (z + 1) * mapX]) * yScale;
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

Model GetSkybox(const char *skyName) {
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
    Rectangle srcRec = (Rectangle){0, 0, size, size};
    Rectangle dstRec = (Rectangle){0, (float)size * (float)i,
                     size, size};
    ImageDraw(&faces, img, srcRec, dstRec, WHITE);
  }
  skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = LoadTextureCubemap(
      faces, CUBEMAP_LAYOUT_AUTO_DETECT); // CUBEMAP_LAYOUT_PANORAMA
  UnloadImage(img);
  UnloadImage(faces);
  return skybox;
}

float GetAdjustedPosition(Vector3 oldPos, HeightMap *heightMap) {
    int indexX = floor(oldPos.x);
    int indexZ = floor(oldPos.z);
    if (indexX >= heightMap->width || indexZ >= heightMap->height){
      // we are out of bounds
      return 0.0f;
    }
    Vector3 a, b, c; // three vectors constructed around oldPos
    Vector3 barycenter; // u, v, w calculated from a, b, c
    float answer;
    if (oldPos.x <= 1 - oldPos.z) {
      a = (Vector3){indexX, heightMap->value[indexX * heightMap->width + indexZ], indexZ};
      b = (Vector3){indexX + 1, heightMap->value[(indexX + 1) * heightMap->width + indexZ], indexZ};
      c = (Vector3){indexX, heightMap->value[indexX * heightMap->width + (indexZ + 1)], indexZ + 1};

      } else {
        a = (Vector3){indexX + 1, heightMap->value[(indexX + 1) * heightMap->width + indexZ], indexZ};
        b = (Vector3){indexX + 1, heightMap->value[(indexX + 1) * heightMap->width + (indexZ + 1)], indexZ + 1};
        c = (Vector3){indexX, heightMap->value[indexX * heightMap->width + (indexZ + 1)], indexZ + 1};
    }
    barycenter = Vector3Barycenter(oldPos, a, b, c);
    answer = barycenter.x * a.y + barycenter.y * b.y + barycenter.z * c.y;
    
    return answer;
}