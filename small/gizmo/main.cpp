// PBR lights with gizmo control

#include "components.h"
#include "systems.h"
#include "flecs.h"
#include "graphics.h"
#include "raygizmo.h"
#include "rlgl.h"
#include <cstdio>
#include <vector>

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    // Scene setup
    ecs.set<Scene>({0.0f});

    // Initialize graphics
    graphics::init(ecs, {800, 600, "Gizmo PBR"});

    // Load PBR shader via LightRenderer singleton
    auto vs = graphics::npath(TextFormat("resources/shaders/glsl%s/pbr.vs", GLSL_VERSION));
    auto fs = graphics::npath(TextFormat("resources/shaders/glsl%s/pbr.fs", GLSL_VERSION));

    // set up lighting — observer activates shader on set
    graphics::LightRenderer ls;
    ls.vs_path = vs.c_str();
    ls.fs_path = fs.c_str();
    ecs.set<graphics::LightRenderer>(ls);

    // shadow mapping — observer activates on set
    ecs.set<graphics::ShadowRenderer>({});

    Shader shader = ecs.get<graphics::LightRenderer>().shader;

    // PBR-specific shader setup (material config, not lighting)
    shader.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(shader, "albedoMap");
    shader.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(shader, "mraMap");
    shader.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(shader, "normalMap");
    shader.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader, "emissiveMap");
    shader.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(shader, "albedoColor");

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

    // flat normal map for PBR (tangent-space up = no perturbation)
    Image flatNormalImg = GenImageColor(4, 4, {128, 128, 255, 255});
    Texture2D flatNormalTexBox = LoadTextureFromImage(flatNormalImg);
    UnloadImage(flatNormalImg);

    rlDisableBackfaceCulling();
    box.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_METALNESS].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_ROUGHNESS].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_EMISSION].texture = whiteTexture;
    box.materials[0].maps[MATERIAL_MAP_NORMAL].texture = flatNormalTexBox;

    // Create floor model
    Model floor = LoadModel(graphics::npath("resources/models/plane.glb").c_str());
    for (int i = 0; i < floor.meshCount; ++i) GenMeshTangents(&floor.meshes[i]);
    // flat normal map (tangent-space up) so TBN works correctly
    Image flatNormal = GenImageColor(4, 4, {128, 128, 255, 255});
    Texture2D flatNormalTex = LoadTextureFromImage(flatNormal);
    UnloadImage(flatNormal);
    floor.materials[0].shader = shader;
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_ROUGHNESS].value = 0.0f;
    floor.materials[0].maps[MATERIAL_MAP_OCCLUSION].value = 1.0f;
    floor.materials[0].maps[MATERIAL_MAP_EMISSION].color = BLACK;
    floor.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = LoadTextureFromImage(GenImageColor(4, 4, GRAY));
    floor.materials[0].maps[MATERIAL_MAP_METALNESS].texture = LoadTextureFromImage(GenImageChecked(10, 10, 1, 1, WHITE, GRAY));
    floor.materials[0].maps[MATERIAL_MAP_NORMAL].texture = flatNormalTex;

    // renderable entities — ECS-visible, tweakable from explorer
    ecs.entity("Box")
        .set<graphics::ModelRef>({&box, 1.0f, {0.5f, 0.5f}})
        .set<graphics::Position>({0, 0, 0})
        .set<graphics::Albedo>({{1, 1, 1, 1}})
        .set<graphics::Surface>({1.0f, 1.0f, 1.0f})
        .set<graphics::Emissive>({graphics::to_rgba({255, 162, 0, 255}), 0.01f})
        .add<graphics::ShadowCaster>();

    ecs.entity("Floor")
        .set<graphics::ModelRef>({&floor, 5.0f, {0.5f, 0.5f}})
        .set<graphics::Position>({0, 0, 0})
        .set<graphics::Albedo>({{1, 1, 1, 1}})
        .set<graphics::Surface>({0.0f, 0.0f, 1.0f})
        .set<graphics::Emissive>({{0, 0, 0, 1}, 0.0f})
        .add<graphics::ShadowCaster>();

    // --- typed light entities ---

    ecs.entity("Light::Ambient")
        .set<graphics::AmbientLight>({{0.4f, 0.45f, 0.5f, 1.0f}, 0.08f});

    auto sun = ecs.entity("Light::Sun")
        .set<graphics::DirectionalLight>({true, graphics::to_rgba({255, 245, 230, 255}), 1.2f, {-0.5f, -1.0f, -0.5f}});

    Transform gt_yellow = GizmoIdentity();
    gt_yellow.translation = {-1.0f, 1.0f, -2.0f};
    auto yellow = ecs.entity("Light::Yellow")
        .set<graphics::PointLight>({true, graphics::to_rgba(YELLOW), 4.0f, 4.0f, 2.0f})
        .set<graphics::Position>({-1.0f, 1.0f, -2.0f})
        .set<graphics::Emissive>({graphics::to_rgba(YELLOW), 1.0f})
        .set<GizmoTransform>({gt_yellow});

    Transform gt_green = GizmoIdentity();
    gt_green.translation = {2.0f, 1.0f, 1.0f};
    auto green = ecs.entity("Light::Green")
        .set<graphics::PointLight>({true, graphics::to_rgba(GREEN), 3.3f, 4.0f, 2.0f})
        .set<graphics::Position>({2.0f, 1.0f, 1.0f})
        .set<graphics::Emissive>({graphics::to_rgba(GREEN), 1.0f})
        .set<GizmoTransform>({gt_green});

    Transform gt_red = GizmoIdentity();
    gt_red.translation = {-2.0f, 1.0f, 1.0f};
    auto red = ecs.entity("Light::Red")
        .set<graphics::PointLight>({true, graphics::to_rgba(RED), 8.3f, 4.0f, 2.0f})
        .set<graphics::Position>({-2.0f, 1.0f, 1.0f})
        .set<graphics::Emissive>({graphics::to_rgba(RED), 1.0f})
        .set<GizmoTransform>({gt_red});

    Transform gt_blue = GizmoIdentity();
    gt_blue.translation = {1.0f, 1.0f, -2.0f};
    auto blue = ecs.entity("Light::Blue")
        .set<graphics::PointLight>({true, graphics::to_rgba(BLUE), 2.0f, 4.0f, 2.0f})
        .set<graphics::Position>({1.0f, 1.0f, -2.0f})
        .set<graphics::Emissive>({graphics::to_rgba(BLUE), 1.0f})
        .set<GizmoTransform>({gt_blue});

    Transform gt_spot = GizmoIdentity();
    gt_spot.translation = {0.0f, 5.0f, 0.0f};
    auto spot = ecs.entity("Light::Spot")
        .set<graphics::SpotLight>({true, {1, 1, 1, 1}, 5.0f, {0, -1, 0}, 15.0f, 2.0f, 25.0f, 35.0f})
        .set<graphics::Position>({0.0f, 5.0f, 0.0f})
        .set<graphics::Emissive>({{1, 1, 1, 1}, 1.0f})
        .set<GizmoTransform>({gt_spot});

    // collect toggleable lights for key bindings
    std::vector<flecs::entity> toggleable = {sun, yellow, green, red, blue, spot};

    bool show_shadow_debug = false;

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
            // toggle lights: [1]=sun [2]=yellow [3]=green [4]=red [5]=blue [6]=spot
            for (int k = 0; k < (int)toggleable.size() && k < 6; ++k) {
                if (IsKeyPressed(KEY_ONE + k)) {
                    auto e = toggleable[k];
                    if (auto* dl = e.try_get_mut<graphics::DirectionalLight>())
                        dl->enabled = !dl->enabled;
                    else if (auto* pl = e.try_get_mut<graphics::PointLight>())
                        pl->enabled = !pl->enabled;
                    else if (auto* sl = e.try_get_mut<graphics::SpotLight>())
                        sl->enabled = !sl->enabled;
                }
            }

            // [7] toggle shadow debug
            if (IsKeyPressed(KEY_SEVEN)) show_shadow_debug = !show_shadow_debug;
        });

    // =========================================================================
    // Shadow pass — queries light + shadow caster entities from ECS
    // =========================================================================
    ecs.system("ShadowPass")
        .kind(graphics::OnShadowPass)
        .run([](flecs::iter& it) {
            auto& ss = it.world().get_mut<graphics::ShadowRenderer>();
            const auto& ls = it.world().get<graphics::LightRenderer>();

            // directional shadow (single FBO — only last enabled light's map survives)
            if (ss.fbo) {
                graphics::queries::directional.each([&](const graphics::DirectionalLight& dl) {
                    if (!dl.enabled) return;
                    ss.begin(dl, 10.0f);
                    graphics::queries::shadow_casters.each([&](const graphics::ModelRef& ref, const graphics::Position& pos) {
                        ss.draw(*ref.model, pos, ref.scale, WHITE);
                    });
                    ss.end(ls);
                });
            }

            // spot shadow (single FBO — only last enabled light's map survives)
            if (ss.spot_fbo) {
                graphics::queries::spot.each([&](const graphics::SpotLight& sl, const graphics::Position& pos) {
                    if (!sl.enabled) return;
                    ss.begin_spot(sl, pos, 0.1f);
                    graphics::queries::shadow_casters.each([&](const graphics::ModelRef& ref, const graphics::Position& pos) {
                        ss.draw(*ref.model, pos, ref.scale, WHITE);
                    });
                    ss.end_spot(ls);
                });
            }
        });

    // =========================================================================
    // Render systems
    // =========================================================================
    ecs.system("DrawModels")
        .kind(graphics::OnRender)
        .run([](flecs::iter& it) {
            const auto& ls = it.world().get<graphics::LightRenderer>();
            const auto& ml = ls.mat_locs;

            graphics::queries::renderables.each([&](flecs::entity e, const graphics::ModelRef& ref, const graphics::Position& pos) {
                SetShaderValue(ml.shader, ml.tiling, &ref.tiling, SHADER_UNIFORM_VEC2);
                ml.upload(e);
                DrawModel(*ref.model, pos, ref.scale, WHITE);
            });
        });

    // gizmos + debug draws in overlay phase (unlit, default shader)
    ecs.system("DrawGizmos")
        .kind(graphics::OnRenderOverlay)
        .run([&](flecs::iter& it) {
            graphics::input::capture_mouse_left = false;

            graphics::input::capture_mouse_left |=
                DrawGizmo3D(GIZMO_TRANSLATE, &boxTransform);
            box.transform = GizmoToMatrix(boxTransform);
        });

    // draw directional light vis (no gizmo, direction only)
    ecs.system<const graphics::DirectionalLight>("DrawDirectionalLights")
        .kind(graphics::OnRenderOverlay)
        .each([&](const graphics::DirectionalLight& dl) {
            systems::draw_directional(dl);
        });

    // draw point light vis + gizmo
    ecs.system<graphics::PointLight, graphics::Position, GizmoTransform>("DrawPointLights")
        .kind(graphics::OnRenderOverlay)
        .each([&](graphics::PointLight& pl, graphics::Position& pos, GizmoTransform& gt) {
            systems::draw_point(pl, pos);
            graphics::input::capture_mouse_left |=
                DrawGizmo3D(GIZMO_TRANSLATE, &gt.value);
            pos.x = gt.value.translation.x;
            pos.y = gt.value.translation.y;
            pos.z = gt.value.translation.z;
        });

    // draw spot light vis + gizmo
    ecs.system<graphics::SpotLight, graphics::Position, GizmoTransform>("DrawSpotLights")
        .kind(graphics::OnRenderOverlay)
        .each([&](graphics::SpotLight& sl, graphics::Position& pos, GizmoTransform& gt) {
            systems::draw_spot(sl, pos);
            graphics::input::capture_mouse_left |=
                DrawGizmo3D(GIZMO_TRANSLATE | GIZMO_ROTATE, &gt.value);
            pos.x = gt.value.translation.x;
            pos.y = gt.value.translation.y;
            pos.z = gt.value.translation.z;
            graphics::quatf q(gt.value.rotation.x, gt.value.rotation.y,
                              gt.value.rotation.z, gt.value.rotation.w);
            Eigen::Vector3f fwd = q.map() * Eigen::Vector3f(0, -1, 0);
            sl.direction = graphics::vec3f(fwd);
        });

    // shadow frustum wireframe (3D overlay)
    ecs.system("DrawShadowDebugFrustum")
        .kind(graphics::OnRenderOverlay)
        .run([&](flecs::iter& it) {
            if (!show_shadow_debug) return;
            const auto& ss = it.world().get<graphics::ShadowRenderer>();
            if (ss.fbo) graphics::ShadowRenderer::debug_frustum(ss.light_vp, {180, 150, 0, 255});
            if (ss.spot_fbo) graphics::ShadowRenderer::debug_frustum(ss.spot_light_vp, {0, 150, 180, 255});
        });

    ecs.system("DrawUI")
        .kind(graphics::PostRender)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            scene.elapsed += it.delta_time();

            DrawText("Toggle: [1]sun [2]yellow [3]green [4]red [5]blue [6]spot [7]shadow", 10, 40, 20, LIGHTGRAY);
            DrawText(TextFormat("elapsed: %.2f", scene.elapsed), 10, 90, 20, GREEN);

            // shadow map overlays (2D, top-right corner)
            if (show_shadow_debug) {
                int size = 256;
                const auto& ss = it.world().get<graphics::ShadowRenderer>();
                if (ss.fbo)
                    graphics::ShadowRenderer::debug_overlay(ss.depth_tex, ss.resolution,
                        GetScreenWidth() - size - 10, 10, size, "dir shadow");
                if (ss.spot_fbo)
                    graphics::ShadowRenderer::debug_overlay(ss.spot_depth_tex, ss.spot_resolution,
                        GetScreenWidth() - size*2 - 20, 10, size, "spot shadow");
            }
        });

    // =========================================================================
    // Main loop
    // =========================================================================
    ecs.app()
        .enable_rest()
        .run();

    // cleanup — observers handle deactivation
    ecs.remove<graphics::ShadowRenderer>();
    ecs.remove<graphics::LightRenderer>();

    floor.materials[0].shader = {0};
    UnloadMaterial(floor.materials[0]);
    floor.materials[0].maps = NULL;
    UnloadModel(floor);

    printf("Simulation ended.\n");
    return 0;
}
