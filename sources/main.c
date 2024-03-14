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

// convenient global for rectangle selection
bool is_select_visible;

// NOTE: need to add input event system, two event buffers

int main(void)
{
  //--------------------------------------------------------------------------
  // Initialization
  //--------------------------------------------------------------------------
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(screenWidth, screenHeight, "raylib - demo");

  // init two camera globals
  is_select_visible = false;

// textures, models, and shaders
#if 0
  Texture2D bill = LoadTexture("../resources/billboard.png");
  Shader alphaDiscard = LoadShader(NULL, "../shaders/alphaDiscard.fs");
  GenTextureMipmaps(&bill);
  SetTextureWrap(bill, TEXTURE_WRAP_CLAMP);
  SetTextureFilter(bill, TEXTURE_FILTER_ANISOTROPIC_16X);
#endif

  Model skybox = skybox_init("../resources/dsky");

  // lighting shader
  Shader mesh_phong = LoadShader("../shaders/sun_light.vs", "../shaders/sun_light.fs");
  mesh_phong.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(mesh_phong, "viewPos");
  mesh_phong.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(mesh_phong, "model");
  mesh_phong.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(mesh_phong, "projection");
  mesh_phong.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(mesh_phong, "view");
  mesh_phong.locs[SHADER_LOC_MATRIX_NORMAL] = GetShaderLocation(mesh_phong, "matNormal");
  
  int ambient_loc = GetShaderLocation(mesh_phong, "ambient");
  SetShaderValue(mesh_phong, ambient_loc, (float[3]){ 0.05f, 0.05f, 0.05f }, SHADER_UNIFORM_VEC3);

  

  Vector3 sun_dir = Vector3Normalize((Vector3){0.3f, -1.f, 0.2f});
  
  

  // depth map
  Shader depth_shader = LoadShader("../shaders/depth_shader.vs", "../shaders/depth_shader.fs");
  depth_shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(depth_shader, "model");
  int depth_loc = GetShaderLocation(depth_shader, "lightSpaceMatrix");


  
  Matrix lightProjection = MatrixOrtho(-50.0f, 50.0f, -50.0f, 50.0f, 0.1f, 50.f);


  Matrix lightView = MatrixLookAt((Vector3){0.0f, 30.0f, 0.0f}, (Vector3){0}, (Vector3){0.0f, 1.0f, 0.0f});
  Matrix lightSpaceMatrix = MatrixMultiply(lightProjection, lightView);

  // lightSpaceMatrix = MatrixTranspose(lightSpaceMatrix);
  //Matrix lightSpace = MatrixMultiply(GetCameraMatrix(sun_camera), MatrixOrtho(-35.0f, 35.0f, -35.0f, 35.0f, RL_CULL_DISTANCE_NEAR, 100.f));
  // SetShaderValueMatrix(depth_shader, depth_loc, lightSpaceMatrix);
  
  
  // entity shadows
  RenderTexture2D shadow_text = {0};
  shadow_text = LoadRenderTextureWithDepthTexture(1024, 1024);
  //SetTextureFilter(shadow_text.depth, TEXTURE_FILTER_TRILINEAR);


  // terrain
  Image disc_map = LoadImage("../resources/discmap.BMP");
  Texture2D color_map = LoadTexture("../resources/colormap.BMP");
  Texture2D shadow_map = LoadTexture("../resources/shadowmap.BMP");
  game_terrain_map_t terrain_map = (game_terrain_map_t){.max_width = disc_map.width, .max_height = disc_map.height};
  Shader terrain_shadow = LoadShader("../shaders/terrain_shadow.vs", "../shaders/terrain_shadow.fs");
  terrain_shadow.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(terrain_shadow, "viewPos");
  SetShaderValueMatrix(terrain_shadow, GetShaderLocation(terrain_shadow, "lightSpaceMatrix"), lightSpaceMatrix);

  int sun_loc[] = {GetShaderLocation(mesh_phong, "sunDir"), GetShaderLocation(terrain_shadow, "sunDir")};
  SetShaderValue(mesh_phong, sun_loc[0], Vector3ToFloat(sun_dir), SHADER_UNIFORM_VEC3);
  SetShaderValue(terrain_shadow, sun_loc[1], Vector3ToFloat(sun_dir), SHADER_UNIFORM_VEC3);
  
  terrain_map.value = RL_MALLOC(sizeof(*terrain_map.value) * disc_map.width * disc_map.height);
  if (!terrain_map.value)
  {
    TraceLog(LOG_ERROR, "Failed to allocate heightmap memory.");
    return EXIT_FAILURE;
  }
  Mesh terrain_mesh = terrain_init(disc_map, &terrain_map);
  Material terrain_material = LoadMaterialDefault();
  terrain_material.shader = terrain_shadow;
  terrain_material.maps[MATERIAL_MAP_DIFFUSE].texture = color_map;
  terrain_material.maps[MATERIAL_MAP_METALNESS].texture = shadow_map;
  terrain_material.maps[MATERIAL_MAP_NORMAL].texture = shadow_text.depth;


  GenTextureMipmaps(&color_map);
  SetTextureWrap(color_map, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(color_map, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(color_map, TEXTURE_FILTER_ANISOTROPIC_16X);

  GenTextureMipmaps(&shadow_map);
  SetTextureWrap(shadow_map, TEXTURE_WRAP_REPEAT);
  SetTextureFilter(shadow_map, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(shadow_map, TEXTURE_FILTER_ANISOTROPIC_16X);

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

  new_ent.position.x = 20;
  entities = entity_add(entities, &new_ent);
  new_ent.position.y = 3;
  entities = entity_add(entities, &new_ent);
  new_ent.position.x = 5;
  entities = entity_add(entities, &new_ent);

  
  short selected[GAME_MAX_SELECTED]; // storing capacity, or maintining a free list might be better, but this works for now
  memset(selected, -1, sizeof selected);


  SetTargetFPS(200);

  game_camera_t camera = {0};
  game_camera_init(&camera, 45.0f, (Vector3){0, 0, 0}, &terrain_map);

  //testing
  //Shader deb = LoadShader("../shaders/debug_quad.vs", "../shaders/debug_quad.fs");

  #if 1
  // assign phong shader + shadow map to all entities
  for (int i = 0; i < arrlen(entities); i++)
  {
    for (int j = 0; j < entities[i].model.materialCount; j++)
    {
      entities[i].model.materials[j].shader = mesh_phong;
    }
  }
  #endif


  

  float sim_accumulator = 0;
  float sim_ai_accumulator = 0;
  float sim_ai_dt = 1.f; // how often ai calculations should be made
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
    SetShaderValue(terrain_shadow, terrain_shadow.locs[SHADER_LOC_VECTOR_VIEW], &camera.ray_view_cam.target, SHADER_UNIFORM_VEC3);
    float dt = GetFrameTime();
    sim_accumulator += dt;
    sim_ai_accumulator += dt;
    while (sim_ai_accumulator >= sim_ai_dt)
    {
      scene_process_ai(entities);
      sim_ai_accumulator -= sim_ai_dt;
    }
    while (sim_accumulator >= sim_dt)
    {
      scene_process_input(&camera, entities, &terrain_map, selected);
      scene_update_entities(&camera, entities, &terrain_map, selected, sim_dt);
      sim_accumulator -= sim_dt;
    }

    //----------------------------------------------------------------------
    // Draw
    //----------------------------------------------------------------------

    // Initializes render texture for drawing

    // First update shadow map
    BeginTextureMode(shadow_text);
    rlClearScreenBuffers();
    rlSetCullFace(RL_CULL_FACE_FRONT);
    rlEnableBackfaceCulling();
    rlEnableDepthTest();
    rlEnableDepthMask();
      for (size_t i = 0; i < arrlen(entities); i++)
      {
        game_entity_t *ent = &entities[i];
        if (ent->type == GAME_ENT_TYPE_ACTOR)
        {
          for (int i = 0; i < ent->model.meshCount; i++)
          {
            Mesh *mesh = &ent->model.meshes[i];
            rlEnableShader(depth_shader.id);
            rlEnableVertexArray(mesh->vaoId);
            rlSetUniformMatrix(depth_loc, lightSpaceMatrix);
            rlSetUniformMatrix(depth_shader.locs[SHADER_LOC_MATRIX_MODEL], ent->model.transform);

            rlDrawVertexArrayElements(0, mesh->triangleCount*3, 0);

            rlDisableVertexArray();
            rlDisableVertexBuffer();
            rlDisableVertexBufferElement();
            rlDisableShader();
          }
        }
      }
    rlSetCullFace(RL_CULL_FACE_BACK);
    EndTextureMode();
    
    // Now Draw scene
    BeginDrawing();


    ClearBackground(RAYWHITE);
    #if 0
    game_camera_begin_mode_3d(&camera);
    BeginShaderMode(terrain_shadow);
    SetShaderValueTexture(deb, GetShaderLocation(deb, "depthMap"), shadow_text.depth);
    DrawCube((Vector3){0}, 1024, 1, 1024, BLUE);
    EndShaderMode();
    game_camera_end_mode_3d();

    #else


    game_camera_begin_mode_3d(&camera);


    // Draw Terrain
    //SetShaderValueTexture(terrain_shadow, shadow_loc, shadow_text.depth);
    DrawMesh(terrain_mesh, terrain_material, terrain_matrix);

    // draw entities
    for (size_t i = 0; i < arrlen(entities); i++)
    {
      game_entity_t *ent = &entities[i];
      #if 0
      for (int j = 0; j < ent->model.materialCount; j++)
      {
          ent->model.materials[j].shader = mesh_phong;
      }   
      #endif
      if (ent->type == GAME_ENT_TYPE_ACTOR)
      {
        Color color_tint = WHITE;
        if (ent->team == GAME_TEAM_PLAYER)
        {
            color_tint = GREEN;
        }
        else if (ent->team == GAME_TEAM_AI)
        {
            color_tint = RED;
        }
        DrawModel(ent->model, (Vector3){0}, 1.0f, color_tint);
        //entity_draw_actor(&ent->model, ent->team);
      }
    }
    // draw selection boxes
    for (size_t i = 0; i < GAME_MAX_SELECTED; i++)
    {
      if (selected[i] >= 0)
      {
        short selected_id = selected[i];
        game_entity_t *ent = &entities[selected_id]; // only works right now, will not work if deleting entities is added since selectedId may not necessarily map to an index
        if (ent->state & GAME_ENT_STATE_DEAD)
        {
          selected[i] = -1; // deselect
          continue;
        }
        DrawCubeWires(Vector3Add(ent->dimensions_offset, Vector3Transform(Vector3Zero(), ent->model.transform)), ent->dimensions.x, ent->dimensions.y, ent->dimensions.z, MAGENTA);
        #if 0
        DrawCircle3D(ent->position, ent->attack_radius, (Vector3){1, 0, 0}, 90, RED);
        #endif
      }
    }

    #if 0
    DrawSphere(camera.camera_pos, 0.25f, RED);
    #endif



    // skybox, to be drawn last
    DrawModel(skybox, (Vector3){0, 0, 0}, 1.0f, GREEN);


    game_camera_end_mode_3d();

    // draw health for selected
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

    // draw rectangle select
    if (is_select_visible)
    {
      Vector2 current_new_pos = GetMousePosition();
      Rectangle box = game_camera_get_mouse_rect(camera.mouse_old_pos, current_new_pos);
      DrawRectangleLines(box.x, box.y, box.width, box.height, GREEN);
    }


    DrawFPS(10, 10);
    DrawText(TextFormat("%.4f\n%.4f\n%05.4f",
                        camera.camera_pos.x,
                        camera.ray_view_cam.position.y,
                        camera.camera_pos.z),
             10, 30, 20, WHITE);
    #endif
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
