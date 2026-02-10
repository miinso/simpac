#include "flecs.h"

#include "graphics.h"

#include "raylib.h"
#include "rlgl.h"

#include <string>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

struct ShaderC {
    Shader shader;
    std::string vert_path;
    std::string frag_path;
    long last_mod_time;

    // uniform locations
    int resolution_loc;
    int mouse_loc;
    int time_loc;

    // shader uniforms
    float resolution[2];
    float mouse_pos[2];
    float total_time;
};

// system to update shader uniforms and check for reloading
void update_shader_system(flecs::iter& it, size_t, ShaderC& sc) {
    sc.total_time += it.delta_time();
    SetShaderValue(sc.shader, sc.time_loc, &sc.total_time, SHADER_UNIFORM_FLOAT);

    Vector2 mouse = GetMousePosition();
    sc.mouse_pos[0] = mouse.x;
    sc.mouse_pos[1] = mouse.y;
    SetShaderValue(sc.shader, sc.mouse_loc, sc.mouse_pos, SHADER_UNIFORM_VEC2);

    long current_mod_time = GetFileModTime(sc.frag_path.c_str());

    if (current_mod_time != sc.last_mod_time) {
        // try reloading shader
        Shader updated_shader = LoadShader(0, sc.frag_path.c_str());

        if (updated_shader.id != rlGetShaderIdDefault()) {
            UnloadShader(sc.shader);
            sc.shader = updated_shader;

            // get new uniform locations
            sc.resolution_loc = GetShaderLocation(sc.shader, "resolution");
            sc.mouse_loc = GetShaderLocation(sc.shader, "mouse");
            sc.time_loc = GetShaderLocation(sc.shader, "time");

            // reset uniforms
            SetShaderValue(sc.shader, sc.resolution_loc, sc.resolution, SHADER_UNIFORM_VEC2);

            TraceLog(LOG_INFO, "shader reloaded successfully");
            
        }

        sc.last_mod_time = current_mod_time;
    }
}

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(688, 344, "Shader Hot Reload Test");

    ecs.observer<ShaderC>().event(flecs::OnSet).each([](flecs::iter& it, size_t i, ShaderC& sc) {
        // initialize shader on component creation
        sc.total_time = 0.0f;

        // load shader
        sc.shader = LoadShader(0, sc.frag_path.c_str());

        if (!IsShaderValid(sc.shader)) {
            TraceLog(LOG_ERROR, "failed to initialize shader for entity: %s", it.entity(i).name().c_str());
            return;
        }

        sc.last_mod_time = GetFileModTime(sc.frag_path.c_str());

        // get uniform locations
        sc.resolution_loc = GetShaderLocation(sc.shader, "resolution");
        sc.mouse_loc = GetShaderLocation(sc.shader, "mouse");
        sc.time_loc = GetShaderLocation(sc.shader, "time");

        // set initial resolution
        sc.resolution[0] = 688.0f;
        sc.resolution[1] = 344.0f;
        SetShaderValue(sc.shader, sc.resolution_loc, sc.resolution, SHADER_UNIFORM_VEC2);

        TraceLog(LOG_INFO, "shader initialized for entity: %s", it.entity(i).name().c_str());
    });

    ecs.observer<ShaderC>().event(flecs::OnRemove).each([](ShaderC& sc) {
        UnloadShader(sc.shader);
        TraceLog(LOG_INFO, "shader unloaded for entity: ");
    });

    // create shader component
    auto shader_entity = ecs.entity("my_shader");
    ShaderC sc = {};
    sc.frag_path = "resources/shaders/glsl300es/reload.fs";
    shader_entity.set<ShaderC>(sc);

    // file watcher system
    ecs.system<ShaderC>().kind(graphics::PreRender).each(update_shader_system);

    ecs.system<const ShaderC>("draw_shader").kind(graphics::PostRender).each([](const ShaderC& shader) {
        if (!IsShaderValid(shader.shader)) {
            // fallback to drawing without shader
            DrawRectangle(0, 0, 688, 344, WHITE);
            DrawText("shader error :(", 10, 10, 20, RED);
            return;
        }

        BeginShaderMode(shader.shader);
        DrawRectangle(0, 0, 688, 344, WHITE);
        EndShaderMode();
    });

    ecs.system("TickTime")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter&) {
            global_time += delta_time;
        });

    graphics::run_loop();

    TraceLog(LOG_INFO, "Simulation ended.");
    return 0;
}
