#pragma once

#include <flecs.h>
#include <string>

struct App {
public:
    App(const char *window_name, int window_width, int window_height);
    void run();
    flecs::world m_world;

private:
    void update_draw_frame_desktop();
    static void update_draw_frame_browser(void *app);

    std::string m_window_name;
    int m_window_width;
    int m_window_height;
};
