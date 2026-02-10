// Minimal config playground (compile-time + runtime props)

#include "graphics.h"

#include <cstdio>
#include <string>
#include <vector>

using Real = float;

template <typename Derived>
struct vec3 {
    Real x = Real(0);
    Real y = Real(0);
    Real z = Real(0);

    vec3() = default;
    vec3(Real x_, Real y_, Real z_) : x(x_), y(y_), z(z_) {}

    Real* data() { return &x; }
    const Real* data() const { return &x; }

    Real& operator[](size_t i) { return data()[i]; }
    const Real& operator[](size_t i) const { return data()[i]; }
};

template <typename T>
inline void register_vec3(flecs::world& ecs) {
    ecs.component<T>()
        .member("x", &T::x)
        .member("y", &T::y)
        .member("z", &T::z);
}

struct vec3f : vec3<vec3f> {
    using vec3<vec3f>::vec3;
};

// for compile-time (types + meta)
struct Scene {
    Real dt = Real(1.0f / 60.0f);
    bool option1 = false;
    Real option2 = Real(0.25f);
    vec3f option3 = {1.0f, 2.0f, 3.0f};

    static void meta(flecs::world& ecs) {
        ecs.component<Scene>()
            .member("dt", &Scene::dt)
            .member("option1", &Scene::option1)
            .member("option2", &Scene::option2)
            .member("option3", &Scene::option3);
    }
};

// for compile-time (types + meta)
struct Thing1 {
    Real option1 = Real(0.75f);
    int option2 = 3;

    static void meta(flecs::world& ecs) {
        ecs.component<Thing1>()
            .member("option1", &Thing1::option1)
            .member("option2", &Thing1::option2);
    }
};


// for compile-time (registration + explicit configurable nodes)
inline void register_components(flecs::world& ecs) {
    register_vec3<vec3f>(ecs);

    ecs.component<Configurable>();

    // for compile-time (meta)
    Scene::meta(ecs);
    Thing1::meta(ecs);

    // for compile-time (storage)
    ecs.component<Scene>()
        .add(flecs::Singleton);
    ecs.component<Thing1>()
        .add(flecs::Singleton);

    // for compile-time (ui binding)
    ecs.entity("Scene::dt").add<Configurable>();
    ecs.entity("Scene::option1").add<Configurable>();
    ecs.entity("Scene::option2").add<Configurable>();
    ecs.entity("Scene::option3").add<Configurable>();
    ecs.entity("Thing1::option1").add<Configurable>();
    ecs.entity("Thing1::option2").add<Configurable>();
}

namespace {
struct ConfigEntry {
    flecs::entity entity;
    std::string parent;
    std::string name;
};

std::vector<ConfigEntry> collect_entries(flecs::world& ecs, bool runtime) {
    std::vector<ConfigEntry> entries;
    auto runtime_root = ecs.entity("Runtime");
    auto builder = ecs.query_builder<Configurable>();

    if (runtime) {
        builder.with(flecs::ChildOf, runtime_root).up(flecs::ChildOf);
    } else {
        builder.without(flecs::ChildOf, runtime_root).up(flecs::ChildOf);
    }

    auto query = builder.build();
    query.each([&](flecs::entity e, const Configurable&) {
        // NOTE: empty components like tags only accept `const` modifier
        entries.push_back({e, e.parent().path().c_str(), e.name().c_str()});
    });

    return entries;
}

std::string format_value(const flecs::entity& e) {
    if (e.has<bool>()) {
        const auto& v = e.get<bool>();
        return v ? "true" : "false";
    }
    if (e.has<int>()) {
        const auto& v = e.get<int>();
        return TextFormat("%d", v);
    }
    if (e.has<Real>()) {
        const auto& v = e.get<Real>();
        return TextFormat("%.4f", v);
    }
    if (e.has<vec3f>()) {
        const auto& v = e.get<vec3f>();
        return TextFormat("(%.2f, %.2f, %.2f)", v.x, v.y, v.z);
    }
    return "-";
}

void draw_group(int x, int& y, const char* title, const std::vector<ConfigEntry>& entries) {
    auto font = graphics::get_font();
    DrawTextEx(font, title, {(float)x, (float)y}, font.baseSize, 0, DARKGRAY);
    y += font.baseSize;

    std::string current_parent;
    for (const auto& entry : entries) {
        if (entry.parent != current_parent) {
            current_parent = entry.parent;
            DrawTextEx(font, current_parent.c_str(), {(float)x, (float)y}, font.baseSize, 0, BLUE);
            y += font.baseSize;
        }

        std::string line = "  " + entry.name + ": " + format_value(entry.entity);
        DrawTextEx(font, line.c_str(), {(float)x + 14, (float)y}, font.baseSize, 0, BLACK);
        y += font.baseSize;
    }
}
} // namespace

int main() {
    printf("Hi from %s\n", __FILE__);

    flecs::world ecs;

    register_components(ecs);
    graphics::init(ecs);
    graphics::init_window({1024, 640, "Config"});

    // for compile-time (data lives in components)
    ecs.set<Scene>({});
    ecs.set<Thing1>({});

    // for compile-time (explicit nodes for ui binding)
    auto scene_dt = ecs.entity("Scene::dt");
    auto scene_option1 = ecs.entity("Scene::option1");
    auto scene_option2 = ecs.entity("Scene::option2");
    auto scene_option3 = ecs.entity("Scene::option3");
    auto thing1_option1 = ecs.entity("Thing1::option1");
    auto thing1_option2 = ecs.entity("Thing1::option2");

    // for compile-time (bridge component values -> nodes)
    ecs.system("SyncConfigValues")
        .kind(flecs::PreUpdate)
        .run([&](flecs::iter& it) {
            auto& scene = it.world().get_mut<Scene>();
            auto& thing1 = it.world().get_mut<Thing1>();
            
            scene_dt.set<Real>(scene.dt);
            scene_option1.set<bool>(scene.option1);
            scene_option2.set<Real>(scene.option2);
            scene_option3.set<vec3f>(scene.option3);
            
            thing1_option1.set<Real>(thing1.option1);
            thing1_option2.set<int>(thing1.option2);
        });

    // for runtime
    ecs.entity("Runtime::Scene::option4")
        .set<bool>(true)
        .add<Configurable>();
    ecs.entity("Runtime::Scene::option5")
        .set<Real>(Real(0.5f))
        .add<Configurable>();
    ecs.entity("Runtime::Scene::option6")
        .set<vec3f>({4.0f, 5.0f, 6.0f})
        .add<Configurable>();

    ecs.entity("Runtime::Thing1::option1")
        .set<Real>(Real(0.9f))
        .add<Configurable>();
    ecs.entity("Runtime::Thing1::option2")
        .set<int>(7)
        .add<Configurable>();

    // for both (script can touch runtime + compile-time)
    const std::string scene_path = graphics::npath("assets/config.flecs");
    if (!scene_path.empty()) {
        auto script = ecs.script("SceneScript").filename(scene_path.c_str()).run();
        if (!script) {
            std::printf("[Scene] Failed to load %s\n", scene_path.c_str());
        } else if (const EcsScript* data = script.try_get<EcsScript>(); data && data->error) {
            std::printf("[Scene] Script error for %s: %s\n", scene_path.c_str(), data->error);
        } else {
            std::printf("[Scene] Loaded %s\n", scene_path.c_str());
        }
    }

    ecs.system("DrawConfig")
        .kind(graphics::PostRender)
        // .after("graphics::render2d")
        .run([&](flecs::iter& it) {
            auto world = it.world();
            const auto compile_entries = collect_entries(world, false);
            const auto runtime_entries = collect_entries(world, true);

            const int left_x = 20;
            const int right_x = left_x + GetScreenWidth()/2;
            int left_y = 24;
            int right_y = 24;

            // for compile-time
            draw_group(left_x, left_y, "Compile-time", compile_entries);
            // for runtime
            draw_group(right_x, right_y, "Runtime", runtime_entries);
        });

    ecs.app()
        .enable_rest()
        .enable_stats()
        .run();

    printf("[%s] App has ended.\n", __FILE__);
    return 0;
}
