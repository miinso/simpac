// PBR lights with gizmo control
// Refactored to follow mass-spring1/boids pattern

#include "components.h"
#include "systems.h"
#include "flecs.h"
#include "graphics.h"
#include "raygizmo.h"
#include "rlgl.h"
#include <cstdio>
#include <vector>

#define GLSL_VERSION "300es"
#define MAX_LIGHTS 4

// Helper to create a light entity
flecs::entity add_light(flecs::world& ecs, Shader shader, int index,
                        LightType type, Vector3 position, Vector3 target,
                        Color color, float intensity) {
    auto e = ecs.entity();

    // Set light data
    e.set<Light>({type, true, position, target, color, intensity});

    // Initialize shader locations
    LightShaderLocs locs;
    systems::init_light_shader_locs(locs, shader, index);
    e.set<LightShaderLocs>(locs);

    // Initialize gizmo transform
    Transform gt = GizmoIdentity();
    gt.translation = position;
    e.set<GizmoTransform>({gt});

    // Initial shader update
    const auto& light = e.get<Light>();
    systems::update_light_shader(shader, light, locs);

    return e;
}

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // Scene setup
    ecs.set<Scene>({0.0f});

    // Initialize graphics
    graphics::init(ecs);
    graphics::init_window(800, 600, "Gizmo PBR");

    // Load PBR shader using npath
    Shader shader = LoadShader(
        graphics::npath(TextFormat("resources/shaders/glsl%s/pbr.vs", GLSL_VERSION)).c_str(),
        graphics::npath(TextFormat("resources/shaders/glsl%s/pbr.fs", GLSL_VERSION)).c_str()
    );

    // Setup shader locations
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
    shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");

    int lightCountLoc = GetShaderLocation(shader, "numOfLights");
    int maxLightCount = MAX_LIGHTS;
    SetShaderValue(shader, lightCountLoc, &maxLightCount, SHADER_UNIFORM_INT);

    // Ambient setup
    float ambientIntensity = 0.02f;
    Color ambientColor = {26, 32, 135, 255};
    Vector3 ambientNorm = {ambientColor.r / 255.0f, ambientColor.g / 255.0f, ambientColor.b / 255.0f};
    SetShaderValue(shader, GetShaderLocation(shader, "ambientColor"), &ambientNorm, SHADER_UNIFORM_VEC3);
    SetShaderValue(shader, GetShaderLocation(shader, "ambient"), &ambientIntensity, SHADER_UNIFORM_FLOAT);

    // Shader parameter locations
    int emissiveIntensityLoc = GetShaderLocation(shader, "emissivePower");
    int emissiveColorLoc = GetShaderLocation(shader, "emissiveColor");
    int textureTilingLoc = GetShaderLocation(shader, "tiling");

    // Create box model
    Mesh boxMesh = GenMeshKnot(2, 2, 10, 160);
    GenMeshTangents(&boxMesh);
    Model box = LoadModelFromMesh(boxMesh);
    Transform boxTransform = GizmoIdentity();

    box.materials[0].shader = shader;
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].color = {255, 162, 0, 255};

    Image img1 = GenImageColor(4, 4, WHITE);
    Texture2D whiteTexture = LoadTextureFromImage(img1);
    UnloadImage(img1);

    rlDisableBackfaceCulling();
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].texture = whiteTexture;

    // Create floor model
    Model floor = LoadModel(graphics::npath("resources/models/plane.glb").c_str());
    floor.materials[0].shader = shader;
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[MATERIAL_MAP_EMISSION].color = BLACK;
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(GenImageColor(4, 4, GRAY));
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTextureFromImage(GenImageChecked(10, 10, 1, 1, WHITE, GRAY));

    Vector2 boxTextureTiling = {0.5f, 0.5f};
    Vector2 floorTextureTiling = {0.5f, 0.5f};

    // Create lights as ECS entities
    std::vector<flecs::entity> lights;
    lights.push_back(add_light(ecs, shader, 0, LIGHT_POINT, {-1.0f, 1.0f, -2.0f}, {0, 0, 0}, YELLOW, 4.0f));
    lights.push_back(add_light(ecs, shader, 1, LIGHT_POINT, {2.0f, 1.0f, 1.0f}, {0, 0, 0}, GREEN, 3.3f));
    lights.push_back(add_light(ecs, shader, 2, LIGHT_POINT, {-2.0f, 1.0f, 1.0f}, {0, 0, 0}, RED, 8.3f));
    lights.push_back(add_light(ecs, shader, 3, LIGHT_POINT, {1.0f, 1.0f, -2.0f}, {0, 0, 0}, BLUE, 2.0f));

    // Texture usage flags
    int usage = 1;
    SetShaderValue(shader, GetShaderLocation(shader, "useTexAlbedo"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexNormal"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexMRA"), &usage, SHADER_UNIFORM_INT);
    SetShaderValue(shader, GetShaderLocation(shader, "useTexEmissive"), &usage, SHADER_UNIFORM_INT);

    // =========================================================================
    // Input system
    // =========================================================================
    ecs.system("HandleInput")
        .kind(flecs::OnUpdate)
        .run([&](flecs::iter& it) {
            // Update camera shader uniform
            auto camera = graphics::detail::camera;
            float cameraPos[3] = {camera.position.x, camera.position.y, camera.position.z};
            SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], cameraPos, SHADER_UNIFORM_VEC3);

            // Toggle lights with number keys
            if (IsKeyPressed(KEY_ONE) && lights.size() > 2) {
                auto& light = lights[2].get_mut<Light>();
                light.enabled = !light.enabled;
            }
            if (IsKeyPressed(KEY_TWO) && lights.size() > 1) {
                auto& light = lights[1].get_mut<Light>();
                light.enabled = !light.enabled;
            }
            if (IsKeyPressed(KEY_THREE) && lights.size() > 3) {
                auto& light = lights[3].get_mut<Light>();
                light.enabled = !light.enabled;
            }
            if (IsKeyPressed(KEY_FOUR) && lights.size() > 0) {
                auto& light = lights[0].get_mut<Light>();
                light.enabled = !light.enabled;
            }
        });

    // =========================================================================
    // Update lights system
    // =========================================================================
    ecs.system<Light, const LightShaderLocs>("UpdateLights")
        .kind(flecs::OnUpdate)
        .each([&](Light& light, const LightShaderLocs& locs) {
            systems::update_light_shader(shader, light, locs);
        });

    // =========================================================================
    // Render systems
    // =========================================================================
    ecs.system("DrawModels")
        .kind(graphics::phase_on_render)
        .run([&](flecs::iter& it) {
            // Floor
            SetShaderValue(shader, textureTilingLoc, &floorTextureTiling, SHADER_UNIFORM_VEC2);
            Vector4 floorEmissive = ColorNormalize(floor.materials[0].maps[MATERIAL_MAP_EMISSION].color);
            SetShaderValue(shader, emissiveColorLoc, &floorEmissive, SHADER_UNIFORM_VEC4);
            DrawModel(floor, {0.0f, 0.0f, 0.0f}, 5.0f, WHITE);

            // Box
            SetShaderValue(shader, textureTilingLoc, &boxTextureTiling, SHADER_UNIFORM_VEC2);
            Vector4 boxEmissive = ColorNormalize(box.materials[0].maps[MATERIAL_MAP_EMISSION].color);
            SetShaderValue(shader, emissiveColorLoc, &boxEmissive, SHADER_UNIFORM_VEC4);
            float emissiveIntensity = 0.01f;
            SetShaderValue(shader, emissiveIntensityLoc, &emissiveIntensity, SHADER_UNIFORM_FLOAT);
            DrawModel(box, {0.0f, 0.0f, 0.0f}, 1.0f, WHITE);

            // Box gizmo
            DrawGizmo3D(GIZMO_TRANSLATE, &boxTransform);
            box.transform = GizmoToMatrix(boxTransform);
        });

    ecs.system<Light, GizmoTransform, const LightShaderLocs>("DrawLights")
        .kind(graphics::phase_on_render)
        .each([&](Light& light, GizmoTransform& gt, const LightShaderLocs& locs) {
            // Draw light visualization
            systems::draw_light(light);

            // Draw gizmo and sync position
            systems::draw_light_gizmo(gt);
            systems::sync_light_from_gizmo(light, gt);
            systems::update_light_shader(shader, light, locs);
        });

    ecs.system("DrawUI")
        .kind(graphics::phase_post_render)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            scene.elapsed += it.delta_time();

            DrawText("Toggle lights: [1][2][3][4]", 10, 40, 20, LIGHTGRAY);
            DrawText(TextFormat("elapsed: %.2f", scene.elapsed), 10, 90, 20, GREEN);
        });

    // =========================================================================
    // Main loop
    // =========================================================================
    graphics::run_main_loop([]{});

    // Cleanup
    floor.materials[0].shader = {0};
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);
    UnloadShader(shader);

    printf("Simulation ended.\n");
    return 0;
}
