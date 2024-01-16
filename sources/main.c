#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

#include "camera.h"
#include "scene.h"
#include "skybox.h"
#include "terrain.h"
#include "models.h"

#define screenWidth 1280
#define screenHeight 720

char *mouse_strings[] = {"LMB", "RMB", "MMB"};

// NOTE: need to add input event system, two event buffers

int main(void)
{
  //--------------------------------------------------------------------------
  // Initialization
  //--------------------------------------------------------------------------
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "raylib - demo");

// textures, models, and shaders
#if 0
  Texture2D bill = LoadTexture("../resources/billboard.png");
  Shader alphaDiscard = LoadShader(NULL, "../shaders/alphaDiscard.fs");
  GenTextureMipmaps(&bill);
  SetTextureWrap(bill, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(bill, TEXTURE_FILTER_ANISOTROPIC_16X);
#endif

  Model skybox = skybox_init("../resources/ACID");

  // lighting shader
  Shader mesh_phong = LoadShader("../shaders/sun_light.vs", "../shaders/sun_light.fs");
  mesh_phong.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(mesh_phong, "viewPos");
  mesh_phong.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(mesh_phong, "model");
  mesh_phong.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(mesh_phong, "projection");
  mesh_phong.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(mesh_phong, "view");
  // mesh_phong.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(mesh_phong, "normal");
  // look into getAttribLocation, can be used for bones in the future
  int ambient_loc = GetShaderLocation(mesh_phong, "ambient");
  SetShaderValue(mesh_phong, ambient_loc, (float[3]){ 0.05f, 0.05f, 0.05f }, SHADER_UNIFORM_VEC3);

  int sun_loc = GetShaderLocation(mesh_phong, "sunDir");
  
  SetShaderValue(mesh_phong, sun_loc, Vector3ToFloat(Vector3Normalize((Vector3){0.2f, -0.0f, 0.5f})), SHADER_UNIFORM_VEC3);


  // terrain
  Image disc_map = LoadImage("../resources/discmap.BMP");
  Texture2D color_map = LoadTexture("../resources/colormap.BMP");
  game_terrain_map_t terrain_map = (game_terrain_map_t){.max_width = disc_map.width, .max_height = disc_map.height};
  terrain_map.value = MemAlloc(sizeof(*terrain_map.value) * disc_map.width * disc_map.height);
  if (!terrain_map.value)
  {
    TraceLog(LOG_ERROR, "Failed to allocate heightmap memory.");
    return EXIT_FAILURE;
  }
  Mesh terrain_mesh = terrain_init(disc_map, &terrain_map);
  Material terrain_material = LoadMaterialDefault();
  terrain_material.maps[MATERIAL_MAP_DIFFUSE].texture = color_map;

  GenTextureMipmaps(&color_map);
  SetTextureWrap(color_map, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(color_map, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(color_map, TEXTURE_FILTER_ANISOTROPIC_16X);

  UnloadImage(disc_map);

  // Terrain Matrix;
  Matrix terrain_matrix = MatrixTranslate(-(terrain_map.max_width / 2.0f), 0.0f, -(terrain_map.max_height / 2.0f));

  // Entity List

  game_entity_t *entities = NULL;
  game_entity_create_t new_ent = (game_entity_create_t){
      .scale = (Vector3){1.0f, 1.0f, 1.0f},
      .position = (Vector2){.x = 0, .y = 0.f},
      .offset_y = 0.0f,
      .dimensions = (Vector3){3, 6, 3},
      .dimensions_offset = (Vector3){0, 3, 0},
      .model_path = "../resources/robot.glb",
      .model_anims_path = "../resources/robot.glb",
      .move_speed = 0.1f,
      .attack_radius = 5.f,
      .attack_damage = 25.f,
      .attack_cooldown_max = 1.75f,
      .hit_points = 100.f,
      .team = GAME_TEAM_PLAYER,
      .type = GAME_ENT_TYPE_ACTOR};
  entities = entity_add(entities, &new_ent);
  new_ent.position.x = 0;
  new_ent.position.y = 10;
  new_ent.team = GAME_TEAM_AI;

  entities = entity_add(entities, &new_ent);

  // hack to assign shader
  for (int i = 0; i < arrlen(entities); i++)
  {
    for (int j = 0; j < entities[i].model.materialCount; j++)
    entities[i].model.materials[j].shader = mesh_phong;
  }

  short selected[GAME_MAX_SELECTED];
  memset(selected, -1, sizeof selected);

  SetTargetFPS(200);

  game_camera_t camera = {};
  game_camera_init(&camera, 45.0f, (Vector3){0, 0, 0}, &terrain_map);

  float sim_accumulator = 0;
  float sim_dt = 1.f / 60.f; // how many times a second calculations should be made

  //--------------------------------------------------------------------------
  // Main game loop
  //--------------------------------------------------------------------------
  while (!WindowShouldClose())
  {
    // Detect window close button or ESC key
    //----------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------
    game_camera_update(&camera, &terrain_map);
    SetShaderValue(mesh_phong, mesh_phong.locs[SHADER_LOC_VECTOR_VIEW], &camera.ray_view_cam.target, SHADER_UNIFORM_VEC3);
    sim_accumulator += GetFrameTime();
    while (sim_accumulator >= sim_dt)
    {
      entity_update_all(&camera, entities, &terrain_map, selected, sim_dt);
      sim_accumulator -= sim_dt;
    }

    //----------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------
    BeginDrawing();

    ClearBackground(RAYWHITE);

    game_camera_begin_mode_3d(&camera);

    // Draw Terrain
    DrawMesh(terrain_mesh, terrain_material, terrain_matrix);

    // TODO: pass model, view, and perspective matrices to shaders when drawing now (will require custom DrawMesh)
    // load and bind new shaders for animations
    // consolidate model transform matrix and entity matrix (can just add pointer/handle to model for entities now)
    // find way to update animations
    // as of right now, entities will own models, their animations, and anim counts

    // draw entities
    for (size_t i = 0; i < arrlen(entities); i++)
    {
      game_entity_t *ent = &entities[i];
      if (ent->type == GAME_ENT_TYPE_ACTOR)
      {
        entity_draw_actor(&ent->model, ent->team);
      }
    }
    // draw selection boxes
    for (size_t i = 0; i < GAME_MAX_SELECTED; i++)
    {
      if (selected[i] >= 0)
      {
        short selected_id = selected[i];
        game_entity_t *ent = &entities[selected_id]; // only works right now, will not work if deleting entities is added since selectedId may not necessarily map to an index
        DrawCubeWires(Vector3Add(ent->dimensions_offset, Vector3Transform(Vector3Zero(), ent->model.transform)), ent->dimensions.x, ent->dimensions.y, ent->dimensions.z, MAGENTA);
        #if 0
        DrawCircle3D(ent->position, ent->attack_radius, (Vector3){1, 0, 0}, 90, RED);
        #endif
      }
    }

    DrawSphere(camera.camera_pos, 0.25f, RED);

#if 0
    BeginShaderMode(alphaDiscard);

    EndShaderMode();
#endif



    // skybox, to be drawn last
    DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, GREEN);

    game_camera_end_mode_3d();

    for (size_t i = 0; i < GAME_MAX_SELECTED; i++)
    {
      if (selected[i] >= 0)
      {
        short selected_id = selected[i];
        game_entity_t *ent = &entities[selected_id];
        Vector2 pos = GetWorldToScreen((Vector3){ent->bbox.min.x, ent->bbox.max.y, ent->bbox.min.z}, camera.ray_view_cam);
        DrawText(TextFormat("%.0f/%.0f", ent->hit_points, ent->hit_points_max), (int)pos.x, (int)pos.y, 20, BLUE);
      }
    }

    DrawFPS(10, 10);
    DrawText(TextFormat("%.4f\n%.4f\n%05.4f",
                        camera.camera_pos.x,
                        camera.camera_pos.y,
                        camera.camera_pos.z),
             10, 30, 20, WHITE);

    EndDrawing();
  }

  //--------------------------------------------------------------------------
  // De-Initialization
  //--------------------------------------------------------------------------

  UnloadShader(skybox.materials[0].shader);
  UnloadTexture(skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture);
  UnloadModel(skybox);

  // arrfree(meshes);
  // arrfree(mats);
  UnloadMesh(terrain_mesh);
  UnloadTexture(terrain_material.maps[MATERIAL_MAP_DIFFUSE].texture);
  UnloadMaterial(terrain_material);
  MemFree(terrain_map.value);

  // Free entities here
  entity_unload_all(entities);
  UnloadShader(mesh_phong);

  CloseWindow();

  return 0;
}
