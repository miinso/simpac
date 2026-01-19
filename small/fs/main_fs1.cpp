// lights with gizmo control

#include "flecs.h"

#include "graphics.h"
#include "raygizmo.h"
#include "rlgl.h"
#include <cstdio>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

// using namespace rl;

#define GLSL_VERSION "300es"
#define MAX_LIGHTS 4 // Max dynamic lights supported by shader

typedef enum { LIGHT_DIRECTIONAL = 0, LIGHT_POINT, LIGHT_SPOT } LightType;

// Light data
typedef struct {
    int type;
    int enabled;
    Vector3 position;
    Vector3 target;
    float color[4];
    float intensity;

    // Shader light parameters locations
    int typeLoc;
    int enabledLoc;
    int positionLoc;
    int targetLoc;
    int colorLoc;
    int intensityLoc;

    Transform transform;
} Light;

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
static int lightCount = 0; // Current number of dynamic lights that have been created

//----------------------------------------------------------------------------------
// Module specific Functions Declaration
//----------------------------------------------------------------------------------
// Create a light and get shader locations
static Light CreateLight(
    int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader);

// Update light properties on shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light);

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 600, "Gizmo test");

    // Load PBR shader and setup all required locations
    Shader shader = LoadShader(TextFormat("resources/shaders/glsl%s/pbr.vs", GLSL_VERSION),
                               TextFormat("resources/shaders/glsl%s/pbr.fs", GLSL_VERSION));
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    // WARNING: Metalness, roughness, and ambient occlusion are all packed into a MRA texture
    // They are passed as to the SHADER_LOC_MAP_METALNESS location for convenience,
    // shader already takes care of it accordingly
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    // WARNING: Similar to the MRA map, the emissive map packs different information
    // into a single texture: it stores height and emission data
    // It is binded to SHADER_LOC_MAP_EMISSION location an properly processed on shader
    shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
    shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");

    // Setup additional required shader locations, including lights data
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int lightCountLoc = GetShaderLocation(shader, "numOfLights");
    int maxLightCount = MAX_LIGHTS;
    SetShaderValue(shader, lightCountLoc, &maxLightCount, SHADER_UNIFORM_INT);

    // Setup ambient color and intensity parameters
    float ambientIntensity = 0.02f;
    Color ambientColor = Color{26, 32, 135, 255};
    Vector3 ambientColorNormalized =
        Vector3{ambientColor.r / 255.0f, ambientColor.g / 255.0f, ambientColor.b / 255.0f};
    SetShaderValue(shader,
                   GetShaderLocation(shader, "ambientColor"),
                   &ambientColorNormalized,
                   SHADER_UNIFORM_VEC3);
    SetShaderValue(
        shader, GetShaderLocation(shader, "ambient"), &ambientIntensity, SHADER_UNIFORM_FLOAT);

    // Get location for shader parameters that can be modified in real time
    int emissiveIntensityLoc = GetShaderLocation(shader, "emissivePower");
    int emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
    int textureTilingLoc = GetShaderLocation(shader, "tiling");

    // Load old car model using PBR maps and shader
    // WARNING: We know this model consists of a single model.meshes[0] and
    // that model.materials[0] is by default assigned to that mesh
    // There could be more complex models consisting of multiple meshes and
    // multiple materials defined for those meshes... but always 1 mesh = 1 material
    // Model car = LoadModel("resources/models/old_car_new.glb");

    // // Assign already setup PBR shader to model.materials[0], used by models.meshes[0]
    // car.materials[0].shader = shader;

    // // Setup materials[0].maps default parameters
    // car.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    // car.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    // car.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    // car.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    // car.materials[0].maps[MATERIAL_MAP_EMISSION].color = Color{255, 162, 0, 255};

    // // Setup materials[0].maps default textures
    // car.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/old_car_d.png");
    // car.materials[0].maps[MATERIAL_MAP_METALNESS].texture =
    //     LoadTexture("resources/old_car_mra.png");
    // car.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/old_car_n.png");
    // car.materials[0].maps[MATERIAL_MAP_EMISSION].texture = LoadTexture("resources/old_car_e.png");

    // Mesh boxMesh = GenMeshCube(1,1,1);
    Mesh boxMesh = GenMeshKnot(2, 2, 10, 160);
    GenMeshTangents(&boxMesh);
    Model box = LoadModelFromMesh(boxMesh);
    Transform boxTransform = GizmoIdentity();

    // UnloadMesh(boxMesh);
    box.materials[0].shader = shader;
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].color = Color{255, 162, 0, 255};

    Image img1 = GenImageColor(4, 4, WHITE);
    Texture2D whiteTexture = LoadTextureFromImage(img1);
    // UnloadImage(img1);
    rlDisableBackfaceCulling();
    // Setup materials[0].maps default textures
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = whiteTexture;
    // box.materials[0].maps[MATERIAL_MAP_NORMAL].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].texture = whiteTexture;

    // Load floor model mesh and assign material parameters
    // NOTE: A basic plane shape can be generated instead of being loaded from a model file
    Model floor = LoadModel("resources/models/plane.glb");
    //Mesh floorMesh = GenMeshPlane(10, 10, 10, 10);
    //GenMeshTangents(&floorMesh);      // TODO: Review tangents generation
    //Model floor = LoadModelFromMesh(floorMesh);

    // Assign material shader for our floor model, same PBR shader
    floor.materials[0].shader = shader;

    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[MATERIAL_MAP_EMISSION].color = BLACK;

    // floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTexture("resources/road_a.png");
    // floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTexture("resources/road_mra.png");
    // floor.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTexture("resources/road_n.png");
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture =
        LoadTextureFromImage(GenImageColor(4, 4, GRAY));
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture =
        LoadTextureFromImage(GenImageChecked(10, 10, 1, 1, WHITE, GRAY));
    // floor.materials[0].maps[MATERIAL_MAP_NORMAL].texture = LoadTextureFromImage(GenImageWhiteNoise(4, 4, 0.2));

    // Models texture tiling parameter can be stored in the Material struct if required (CURRENTLY NOT USED)
    // NOTE: Material.params[4] are available for generic parameters storage (float)
    // Vector2 carTextureTiling = Vector2{0.5f, 0.5f};
    Vector2 boxTextureTiling = Vector2{0.5f, 0.5f};
    Vector2 floorTextureTiling = Vector2{0.5f, 0.5f};

    // Create some lights
    Light lights[MAX_LIGHTS] = {0};
    lights[0] = CreateLight(
        LIGHT_POINT, Vector3{-1.0f, 1.0f, -2.0f}, Vector3{0.0f, 0.0f, 0.0f}, YELLOW, 4.0f, shader);
    lights[1] = CreateLight(
        LIGHT_POINT, Vector3{2.0f, 1.0f, 1.0f}, Vector3{0.0f, 0.0f, 0.0f}, GREEN, 3.3f, shader);
    lights[2] = CreateLight(
        LIGHT_POINT, Vector3{-2.0f, 1.0f, 1.0f}, Vector3{0.0f, 0.0f, 0.0f}, RED, 8.3f, shader);
    lights[3] = CreateLight(
        LIGHT_POINT, Vector3{1.0f, 1.0f, -2.0f}, Vector3{0.0f, 0.0f, 0.0f}, BLUE, 2.0f, shader);

    // Setup material texture maps usage in shader
    // NOTE: By default, the texture maps are always used
    int usage = 1;
    SetShaderValue(shader, GetShaderLocation(shader, "useTexAlbedo"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexNormal"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexMRA"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexEmissive"), &usage, SHADER_UNIFORM_INT);

    time_t fragShaderFileModTime = 0;

    ecs.system("mouse update").kind(flecs::OnUpdate).run([&](flecs::iter& it) {
        // Update the shader with the camera view vector (points towards { 0.0f, 0.0f, 0.0f })
        auto camera = graphics::detail::camera;
        float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

        // Check key inputs to enable/disable lights
        if (IsKeyPressed(KEY_ONE)) {
            lights[2].enabled = !lights[2].enabled;
        }
        if (IsKeyPressed(KEY_TWO)) {
            lights[1].enabled = !lights[1].enabled;
        }
        if (IsKeyPressed(KEY_THREE)) {
            lights[3].enabled = !lights[3].enabled;
        }
        if (IsKeyPressed(KEY_FOUR)) {
            lights[0].enabled = !lights[0].enabled;
        }

        long currentFragShaderModTime = GetFileModTime("/resources/shaders/glsl300es/pbr.fs");

        // Check if shader file has been modified
        if (currentFragShaderModTime != fragShaderFileModTime) {
            TraceLog(LOG_INFO, "[pbr.fs] file changed");
            fragShaderFileModTime = currentFragShaderModTime;
        }

        // Update light values on shader (actually, only enable/disable them)
        for (int i = 0; i < MAX_LIGHTS; i++)
            UpdateLight(shader, lights[i]);
    });

    ecs.system("draw 3d").kind(graphics::phase_on_render).run([&](flecs::iter& it) {
        // ClearBackground(BLACK);
        // BeginShaderMode(shader);
        // Set floor model texture tiling and emissive color parameters on shader
        SetShaderValue(shader, textureTilingLoc, &floorTextureTiling, SHADER_UNIFORM_VEC2);
        Vector4 floorEmissiveColor =
            ColorNormalize(floor.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        SetShaderValue(shader, emissiveColorLoc, &floorEmissiveColor, SHADER_UNIFORM_VEC4);

        DrawModel(floor, Vector3{0.0f, 0.0f, 0.0f}, 5.0f, WHITE); // Draw floor model

        // Set old car model texture tiling, emissive color and emissive intensity parameters on shader
        // SetShaderValue(shader, textureTilingLoc, &carTextureTiling, SHADER_UNIFORM_VEC2);
        // Vector4 carEmissiveColor =
        //     ColorNormalize(car.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        // SetShaderValue(shader, emissiveColorLoc, &carEmissiveColor, SHADER_UNIFORM_VEC4);

        // Set box model texture tiling, emissive color and emissive intensity parameters on shader
        SetShaderValue(shader, textureTilingLoc, &boxTextureTiling, SHADER_UNIFORM_VEC2);
        Vector4 boxEmissiveColor =
            ColorNormalize(box.materials[0].maps[MATERIAL_MAP_EMISSION].color);
        SetShaderValue(shader, emissiveColorLoc, &boxEmissiveColor, SHADER_UNIFORM_VEC4);

        float emissiveIntensity = 0.01f;
        SetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, SHADER_UNIFORM_FLOAT);

        // DrawModel(car, Vector3{0.0f, 0.0f, 0.0f}, 0.25f, WHITE); // Draw car model
        DrawModel(box, Vector3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE); // Draw box model
        DrawGizmo3D(GIZMO_TRANSLATE, &boxTransform);
        box.transform = GizmoToMatrix(boxTransform);

        // EndShaderMode();

        // Draw spheres to show the lights positions
        for (int i = 0; i < MAX_LIGHTS; i++) {
            Color lightColor =
                Color{static_cast<unsigned char>(lights[i].color[0] * 255),
                      static_cast<unsigned char>(lights[i].color[1] * 255),
                      static_cast<unsigned char>(lights[i].color[2] * 255),
                      static_cast<unsigned char>(lights[i].color[3] * 255)};

            if (lights[i].enabled)
                DrawSphereEx(lights[i].position, 0.2f, 8, 8, lightColor);
            else
                DrawSphereWires(lights[i].position, 0.2f, 8, 8, ColorAlpha(lightColor, 0.3f));

            // draw, handle interaction
            DrawGizmo3D(GIZMO_TRANSLATE, &lights[i].transform);

            // apply gizmo transform to the light's
            lights[i].position = lights[i].transform.translation;
            UpdateLight(shader, lights[i]);
        }
    });

    ecs.system("DrawTimingInfo").kind(graphics::phase_post_render).run([&](flecs::iter& it) {
        DrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);

        DrawText(TextFormat("elapsed: %f", global_time), 10, 90, 20, GREEN);
    });

    ecs.system("TickTime")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter&) {
            global_time += delta_time;
        });

    graphics::run_loop();

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unbind (disconnect) shader from car.material[0]
    // to avoid UnloadMaterial() trying to unload it automatically
    // car.materials[0].shader = Shader{0};
    // UnloadMaterial(car.materials[0]);
    // car.materials[0].maps = NULL;
    // UnloadModel(car);

    floor.materials[0].shader = Shader{0};
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);

    UnloadShader(shader); // Unload Shader

    printf("Simulation ended.\n");
    return 0;
}

// Create light with provided data
// NOTE: It updated the global lightCount and it's limited to MAX_LIGHTS
static Light CreateLight(
    int type, Vector3 position, Vector3 target, Color color, float intensity, Shader shader) {
    Light light = {0};

    if (lightCount < MAX_LIGHTS) {
        light.enabled = 1;
        light.type = type;
        light.position = position;
        light.target = target;
        light.color[0] = (float)color.r / 255.0f;
        light.color[1] = (float)color.g / 255.0f;
        light.color[2] = (float)color.b / 255.0f;
        light.color[3] = (float)color.a / 255.0f;
        light.intensity = intensity;

        // NOTE: Shader parameters names for lights must match the requested ones
        light.enabledLoc = GetShaderLocation(shader, TextFormat("lights[%i].enabled", lightCount));
        light.typeLoc = GetShaderLocation(shader, TextFormat("lights[%i].type", lightCount));
        light.positionLoc =
            GetShaderLocation(shader, TextFormat("lights[%i].position", lightCount));
        light.targetLoc = GetShaderLocation(shader, TextFormat("lights[%i].target", lightCount));
        light.colorLoc = GetShaderLocation(shader, TextFormat("lights[%i].color", lightCount));
        light.intensityLoc =
            GetShaderLocation(shader, TextFormat("lights[%i].intensity", lightCount));

        UpdateLight(shader, light);

        light.transform = GizmoIdentity();
        light.transform.translation = position;

        lightCount++;
    }

    return light;
}

// Send light properties to shader
// NOTE: Light shader locations should be available
static void UpdateLight(Shader shader, Light light) {
    SetShaderValue(shader, light.enabledLoc, &light.enabled, SHADER_UNIFORM_INT);
    SetShaderValue(shader, light.typeLoc, &light.type, SHADER_UNIFORM_INT);

    // Send to shader light position values
    Vector3 position = light.transform.translation;

    float positionValues[3] = {position.x, position.y, position.z};
    SetShaderValue(shader, light.positionLoc, positionValues, SHADER_UNIFORM_VEC3);

    // Send to shader light target position values
    float target[3] = {light.target.x, light.target.y, light.target.z};
    SetShaderValue(shader, light.targetLoc, target, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, light.colorLoc, light.color, SHADER_UNIFORM_VEC4);
    SetShaderValue(shader, light.intensityLoc, &light.intensity, SHADER_UNIFORM_FLOAT);
}
