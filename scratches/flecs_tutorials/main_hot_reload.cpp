#include "flecs.h"

#include "graphics.h"

#include <iostream>
#include <memory>

float randf(int n) {
    return static_cast<float>(rand() % n);
}

float global_time = 0.0f;
float delta_time = 0.016f;

struct ShaderComponent {
    // rl::Shader shader;
    std::shared_ptr<rl::Shader> shader;
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
void update_shader_system(flecs::iter& it, size_t, ShaderComponent& sc) {
    sc.total_time += it.delta_time();
    // rl::SetShaderValue(sc.shader, sc.time_loc, &sc.total_time, rl::SHADER_UNIFORM_FLOAT);
    sc.shader->SetValue(sc.time_loc, &sc.total_time, SHADER_UNIFORM_FLOAT);

    rl::Vector2 mouse = GetMousePosition();
    sc.mouse_pos[0] = mouse.x;
    sc.mouse_pos[1] = mouse.y;
    // rl::SetShaderValue(sc.shader, sc.mouse_loc, sc.mouse_pos, rl::SHADER_UNIFORM_VEC2);
    sc.shader->SetValue(sc.mouse_loc, sc.mouse_pos, SHADER_UNIFORM_VEC2);

    long current_mod_time = rl::GetFileModTime(sc.frag_path.c_str());

    if (current_mod_time != sc.last_mod_time) {
        // try reloading shader
        // rl::Shader updated_shader = rl::LoadShader(0, sc.frag_path.c_str());
        sc.shader = std::make_shared<raylib::Shader>();
        sc.shader->Load("", sc.frag_path);

        if (sc.shader->id != rlGetShaderIdDefault()) {
            // rl::UnloadShader(sc.shader);
            sc.shader->Unload();
            // sc.shader = updated_shader;

            // get new uniform locations
            sc.resolution_loc = sc.shader->GetLocation("resolution");
            sc.mouse_loc = sc.shader->GetLocation("mouse");
            sc.time_loc = sc.shader->GetLocation("time");

            // reset uniforms
            // rl::SetShaderValue(
            //     sc.shader, sc.resolution_loc, sc.resolution, rl::SHADER_UNIFORM_VEC2);
            sc.shader->SetValue(sc.resolution_loc, sc.resolution, SHADER_UNIFORM_VEC2);

            std::cout << "shader reloaded successfully" << std::endl;
        }

        sc.last_mod_time = current_mod_time;
    }
}

int main() {
    flecs::world ecs;

    graphics::init(ecs);
    graphics::init_window(800, 800, "Shader Test");

    ecs.observer<ShaderComponent>()
        .event(flecs::OnSet)
        .each([](flecs::iter& it, size_t i, ShaderComponent& sc) {
            // initialize shader on component creation
            sc.total_time = 0.0f;

            // load shader
            // sc.shader = LoadShader(0, sc.frag_path.c_str());
            sc.shader = std::make_shared<raylib::Shader>();
            sc.shader->Load("", sc.frag_path);

            if (sc.shader->IsValid()) {
                std::cout << "failed to initialize shader for entity: " << it.entity(i).name()
                          << std::endl;
                return;
            }

            sc.last_mod_time = GetFileModTime(sc.frag_path.c_str());

            // get uniform locations
            sc.resolution_loc = sc.shader->GetLocation("resolution");
            sc.mouse_loc = sc.shader->GetLocation("mouse");
            sc.time_loc = sc.shader->GetLocation("time");

            // set initial resolution
            sc.resolution[0] = 800.0f;
            sc.resolution[1] = 800.0f;
            // rl::SetShaderValue(sc.shader, sc.resolution_loc, sc.resolution, rl::SHADER_UNIFORM_VEC2);
            sc.shader->SetValue(sc.resolution_loc, sc.resolution, SHADER_UNIFORM_VEC2);

            std::cout << "shader initialized for entity: " << it.entity(i).name() << std::endl;
        });

    // ecs.observer<ShaderComponent>()
    //     .event(flecs::OnRemove)
    //     .each([](flecs::entity& e, ShaderComponent& sc) {
    //         // rl::UnloadShader(sc.shader);
    //         sc.shader->Unload();
    //         std::cout << "shader unloaded for entity: " << e.name() << std::endl;
    //     });

    // create shader component
    auto shader_entity = ecs.entity("my_shader");
    ShaderComponent sc = {};
    sc.shader = std::make_shared<raylib::Shader>();
    sc.frag_path = "resources/shaders/glsl330/reload.fs";
    shader_entity.set<ShaderComponent>(sc);

    // file watcher system
    ecs.system<ShaderComponent>().kind(graphics::PreRender).each(update_shader_system);

    ecs.system<const ShaderComponent>("draw_shader")
        .kind(graphics::PostRender)
        .each([](const ShaderComponent& shader) {
            if (!shader.shader->IsValid()) {
                // fallback to drawing without shader
                DrawRectangle(0, 0, 800, 800, rl::WHITE);
                raylib::DrawText("shader error :(", 10, 10, 20, rl::RED);
                return;
            }
            // BeginShaderMode(shader.shader);

            shader.shader->BeginMode();
            DrawRectangle(0, 0, 800, 800, rl::WHITE);
            EndShaderMode();
        });

    graphics::run_main_loop([]() { global_time += delta_time; });

    std::cout << "Simulation ended." << std::endl;
    return 0;
}