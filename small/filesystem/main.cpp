#include <flecs.h>
#include <raylib.h>

#include <string>

#include "graphics.h"

struct Watchable {
    std::string path;
    long last_mod = 0;
    std::string content;
};

static std::string read_text_file(const std::string& path) {
    char* text_raw = LoadFileText(path.c_str());
    std::string text = text_raw ? text_raw : "(missing text)";
    if (text_raw) {
        UnloadFileText(text_raw);
    }
    return text;
}

int main() {
    flecs::world world;

    graphics::init(world);
    graphics::init_window(800, 600, "Filesystem Demo");

    std::string asset_path = graphics::npath("assets/myfile.txt");
    std::string asset_text = read_text_file(asset_path);

    std::string asset_path2 = graphics::npath("orphan.txt");
    std::string asset_text2 = read_text_file(asset_path2);

    std::string texture_path = graphics::npath("resources/generic.png");
    Texture2D logo = LoadTexture(texture_path.c_str());

    std::string local_texture_path = graphics::npath("assets/spin.jpg");
    Texture2D image = LoadTexture(local_texture_path.c_str());

    auto watched = world.entity("WatchedFile")
        .set<Watchable>({
            graphics::npath("assets/watched.txt"),
            0,
            "(waiting for file)",
        });

    world.system<Watchable>("WatchFileChanges")
        .kind(flecs::OnUpdate)
        .each([](Watchable& watch) {
            long mod_time = GetFileModTime(watch.path.c_str());
            if (mod_time == 0 || mod_time == watch.last_mod) {
                return;
            }
            watch.last_mod = mod_time;
            watch.content = read_text_file(watch.path);
        });

    world.system("DrawOverlay")
        .kind(graphics::phase_post_render)
        .run([&](flecs::iter&) {
            DrawText("Project asset:", 20, 40, 20, DARKGRAY);
            DrawText(asset_text.c_str(), 20, 70, 20, DARKGREEN);
            DrawText("Root resource:", 20, 120, 20, DARKGRAY);
            DrawTexture(logo, 20, 150, WHITE);
            DrawText("Project resource:", 220, 120, 20, DARKGRAY);
            DrawTexture(image, 220, 150, WHITE);
            const Watchable* watch = watched.try_get<Watchable>();
            DrawText("Watched file:", 220, 40, 20, DARKGRAY);
            if (watch) {
                DrawText(watch->content.c_str(), 220, 60, 20, MAROON);
            }
        });

    world.app().run();

    UnloadTexture(image);
    UnloadTexture(logo);
    return 0;
}
