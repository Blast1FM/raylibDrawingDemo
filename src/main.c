#include "raylib.h"
#include "raymath.h"

#define RLIGHTS_IMPLEMENTATION
#include "rlight.h"

#define GLSL_VERSION 330

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1280;
    const int screenHeight = 720;

    SetConfigFlags(FLAG_MSAA_4X_HINT);

    InitWindow(screenWidth, screenHeight, "IT2");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ 20.0f, 10.0f, 20.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };     // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;                   // Camera mode type

    Model model = LoadModel("assets/obj/cottage_obj.obj");                 // Load model
    Texture2D texture = LoadTexture("assets/textures/cottage_diffuse.png"); // Load model texture
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;            // Set map diffuse texture

    Vector3 position = { 0.0f, 0.0f, 0.0f };                    // Set model position
    float modelScale = 1.0f;

    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;

    BoundingBox bounds = GetMeshBoundingBox(model.meshes[0]);   // Set model bounds

    bool selected = false;          // Selected object flag
    bool drawModel = true;

    // ####################         THE LAND OF SHADERS             #############################
    // Load basic lighting shader
    Shader shader = LoadShader(TextFormat("assets/shaders/lighting.vs", GLSL_VERSION),
                               TextFormat("assets/shaders/lighting.fs", GLSL_VERSION));

    // Get some required shader locations
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    // NOTE: "matModel" location name is automatically assigned on shader loading, 
    // no need to get the location again if using that uniform name
    //shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader, "matModel");
    
    // Ambient light level (some basic lighting)
    int ambientLoc = GetShaderLocation(shader, "ambient");
    SetShaderValue(shader, ambientLoc, (float[4]){ 1.0f, 1.0f, 1.0f, 1.0f }, SHADER_UNIFORM_VEC4);

    model.materials[0].shader = shader;

    // Create lights
    Light lights[MAX_LIGHTS] = { 0 };
    lights[0] = CreateLight(LIGHT_POINT, (Vector3){-2, 1, -2 }, Vector3Zero(), YELLOW, shader);
    lights[1] = CreateLight(LIGHT_POINT, (Vector3){ 2, 1,  2 }, Vector3Zero(), RED, shader);
    lights[2] = CreateLight(LIGHT_POINT, (Vector3){-2, 1,  2 }, Vector3Zero(), GREEN, shader);
    lights[3] = CreateLight(LIGHT_POINT, (Vector3){ 2, 1, -2 }, Vector3Zero(), BLUE, shader);

    // NOTE: bounds are calculated from the original size of the model,
    // if model is scaled on drawing, bounds must be also scaled

    //DisableCursor();                // Limit cursor to relative movement inside the window

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_ORBITAL);

        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        float cameraPos[3] = { camera.position.x, camera.position.y, camera.position.z };
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);
        
        model.transform = MatrixRotateXYZ((Vector3){ DEG2RAD*pitch, DEG2RAD*yaw, DEG2RAD*roll });

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_Y)) { lights[0].enabled = !lights[0].enabled; }
        if (IsKeyPressed(KEY_R)) { lights[1].enabled = !lights[1].enabled; }
        if (IsKeyPressed(KEY_G)) { lights[2].enabled = !lights[2].enabled; }
        if (IsKeyPressed(KEY_B)) { lights[3].enabled = !lights[3].enabled; }
        if (IsKeyPressed(KEY_D)) { drawModel = !drawModel; }
        if (IsKeyPressed(KEY_Q))   {modelScale += 0.1f;   }
        if (IsKeyPressed(KEY_A)) {modelScale -= 0.1f;   }

        if (IsKeyDown(KEY_DOWN)) pitch += 0.6f;
        else if (IsKeyDown(KEY_UP)) pitch -= 0.6f;
        
        if (pitch > 0.3f) pitch -= 0.3f;
        else if (pitch < -0.3f) pitch += 0.3f;

        if (IsKeyDown(KEY_Z)) yaw -= 1.0f;
        else if (IsKeyDown(KEY_X)) yaw += 1.0f;

        if (yaw > 0.0f) yaw -= 0.5f;
        else if (yaw < 0.0f) yaw += 0.5f;

        if (IsKeyDown(KEY_LEFT)) roll -= 1.0f;
        else if (IsKeyDown(KEY_RIGHT)) roll += 1.0f;

        if (roll > 0.0f) roll -= 0.5f;
        else if (roll < 0.0f) roll += 0.5f;

        // Update light values (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++) UpdateLightValues(shader, lights[i]);

        // Load new models/textures on drag&drop
        if (IsFileDropped())
        {
            FilePathList droppedFiles = LoadDroppedFiles();

            if (droppedFiles.count == 1) // Only support one file dropped
            {
                if (IsFileExtension(droppedFiles.paths[0], ".obj") ||
                    IsFileExtension(droppedFiles.paths[0], ".gltf") ||
                    IsFileExtension(droppedFiles.paths[0], ".glb") ||
                    IsFileExtension(droppedFiles.paths[0], ".vox") ||
                    IsFileExtension(droppedFiles.paths[0], ".iqm") ||
                    IsFileExtension(droppedFiles.paths[0], ".m3d"))       // Model file formats supported
                {
                    UnloadModel(model);                         // Unload previous model
                    model = LoadModel(droppedFiles.paths[0]);   // Load new model
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture; // Set current map diffuse texture

                    bounds = GetMeshBoundingBox(model.meshes[0]);

                }
                else if (IsFileExtension(droppedFiles.paths[0], ".png"))  // Texture file formats supported
                {
                    // Unload current model texture and load new one
                    UnloadTexture(texture);
                    texture = LoadTexture(droppedFiles.paths[0]);
                    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = texture;
                }
            }

            UnloadDroppedFiles(droppedFiles);    // Unload filepaths from memory
        }

        // Select model on mouse click
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // Check collision between ray and box
            if (GetRayCollisionBox(GetScreenToWorldRay(GetMousePosition(), camera), bounds).hit) selected = !selected;
            else selected = false;
        }
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                BeginShaderMode(shader);

                    
                    DrawCube(Vector3Zero(), 2.0, 2.0, 2.0, WHITE);
                    DrawPlane(Vector3Zero(), (Vector2) {50.0f, 50.0f}, WHITE);

                    if(drawModel)
                    {
                        DrawModel(model, position, modelScale, WHITE);        // Draw 3d model with texture
                    }
                    
                    DrawGrid(20, 10.0f);         // Draw a grid
                
                EndShaderMode();

                if (selected) DrawBoundingBox(bounds, GREEN);   // Draw selection box

                // Draw spheres to show where the lights are
                for (int i = 0; i < MAX_LIGHTS; i++)
                {
                    if (lights[i].enabled) DrawSphereEx(lights[i].position, 0.2f, 8, 8, lights[i].color);
                    else DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lights[i].color, 0.3f));
                }

            EndMode3D();

            if (selected) DrawText("MODEL SELECTED", GetScreenWidth() - 110, 10, 10, GREEN);

            DrawFPS(10, 10);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(texture);     // Unload texture
    UnloadModel(model);         // Unload model
    UnloadShader(shader);       // Unload shader

    CloseWindow();              // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}