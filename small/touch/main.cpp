// touch/mouse position debug test
// draws blue circle at current pointer pos, red circle at last pos

#include <flecs.h>
#include <raylib.h>

#include "graphics.h"

#include <cstdio>

namespace {

struct PointerState {
    float cur_x = 0, cur_y = 0;   // current pointer pos (screen)
    float last_x = 0, last_y = 0; // last released pos
    bool down = false;
    bool has_last = false;
    int touch_count = 0;
};

} // namespace

int main() {
    std::printf("touch test\n");

    flecs::world ecs;

    graphics::init(ecs, {800, 600, "Touch Test"});

    ecs.set<PointerState>({});

    // update pointer state
    ecs.system("UpdatePointer")
        .kind(flecs::PreUpdate)
        .run([](flecs::iter& it) {
            auto& ps = it.world().get_mut<PointerState>();

            ps.touch_count = GetTouchPointCount();

            Vector2 pos;
            if (ps.touch_count > 0) {
                pos = GetTouchPosition(0);
            } else {
                pos = GetMousePosition();
            }

            bool mouse_down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

            if (mouse_down || ps.touch_count > 0) {
                ps.cur_x = pos.x;
                ps.cur_y = pos.y;
                ps.down = true;
            } else if (ps.down) {
                // just released
                ps.last_x = ps.cur_x;
                ps.last_y = ps.cur_y;
                ps.has_last = true;
                ps.down = false;
            }
        });

    // draw circles
    ecs.system("DrawPointer")
        .kind(graphics::PostRender)
        .run([](flecs::iter& it) {
            const auto& ps = it.world().get<PointerState>();

            // last released pos (red)
            if (ps.has_last) {
                DrawCircle((int)ps.last_x, (int)ps.last_y, 20.0f, RED);
                DrawText(TextFormat("last: %.0f, %.0f", ps.last_x, ps.last_y),
                    (int)ps.last_x + 25, (int)ps.last_y - 10, 16, RED);
            }

            // current active pos (blue)
            if (ps.down) {
                DrawCircle((int)ps.cur_x, (int)ps.cur_y, 20.0f, BLUE);
                DrawText(TextFormat("pos: %.0f, %.0f", ps.cur_x, ps.cur_y),
                    (int)ps.cur_x + 25, (int)ps.cur_y - 10, 16, BLUE);
            }

            // mouse pos from shim (always shown)
            Vector2 m = GetMousePosition();
            int sw = GetScreenWidth();
            int sh = GetScreenHeight();
            int rw = GetRenderWidth();
            int rh = GetRenderHeight();
            DrawText(TextFormat("mouse: %.0f, %.0f", m.x, m.y), 20, 20, 16, DARKGRAY);
            DrawText(TextFormat("touch count: %d", ps.touch_count), 20, 40, 16, DARKGRAY);
            DrawText(TextFormat("down: %s", ps.down ? "yes" : "no"), 20, 60, 16, DARKGRAY);
            DrawText(TextFormat("screen: %dx%d", sw, sh), 20, 80, 16, DARKGRAY);
            DrawText(TextFormat("render: %dx%d", rw, rh), 20, 100, 16, DARKGRAY);

            // touch points (green, smaller)
            for (int i = 0; i < ps.touch_count && i < 10; i++) {
                Vector2 tp = GetTouchPosition(i);
                DrawCircle((int)tp.x, (int)tp.y, 12.0f, GREEN);
                DrawText(TextFormat("t%d", i), (int)tp.x + 15, (int)tp.y - 6, 14, GREEN);
            }

            // crosshair at mouse pos
            DrawLine((int)m.x - 15, (int)m.y, (int)m.x + 15, (int)m.y, DARKGRAY);
            DrawLine((int)m.x, (int)m.y - 15, (int)m.x, (int)m.y + 15, DARKGRAY);
        });

    ecs.app().run();

    return 0;
}
