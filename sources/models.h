#pragma once
#include <stdbool.h>
#include "raylib.h"

typedef struct Model Model;
typedef struct Mesh Mesh;
typedef struct Material Material;
typedef struct Matrix Matrix;
typedef struct ModelAnimation ModelAnimation;

void entity_draw_actor(Model *model, int team);

void entity_draw_mesh(Mesh mesh, Material material, Matrix transform);

Model entity_load_model(const char *fileName);

void entity_upload_mesh(Mesh *mesh, bool dynamic);

RenderTexture2D LoadRenderTextureWithDepthTexture(int width, int height);