#pragma once
#include <stdlib.h>
#include "raylib.h"

// intended for all terrain code, everything outside this module should rely on
// world coordinates

typedef struct game_terrain_map_t
{
  int max_width;
  int max_height;
  float *value; // treat as 2d array, may convert to struct later
} game_terrain_map_t;

// modifies X and Z world coordinates to match indices of terrain map lookups
Vector3 terrain_convert_from_world_pos(Vector3 world_pos, game_terrain_map_t *terrain_map);

Vector3 terrain_convert_to_world_pos(Vector3 terrain_pos, game_terrain_map_t *terrain_map);

// takes in world coordinates and gives an approximation of the height using barycentric coordinates
float terrain_get_adjusted_y(Vector3 world_pos, game_terrain_map_t *terrain_map);

// raylib mesh generation, while also populating an array of floats for future lookups
Mesh terrain_init(Image height_image, game_terrain_map_t *terrain_map);

Vector3 terrain_get_ray(Ray ray, game_terrain_map_t *terrain_map, float z_near, float z_far);