#pragma once

#include <flecs.h>
#include <string>

struct App {
public:
    App(std::string window_name, int window_width, int window_height);
    ~App();

    App(const App &) = delete;
    App &operator=(const App &) = delete;
    App(App &&) = delete;
    App &operator=(App &&) = delete;

    void run();

    [[nodiscard]] flecs::world &world() noexcept { return m_world; }
    [[nodiscard]] const flecs::world &world() const noexcept { return m_world; }

private:
    void tick();
    static void update_draw_frame_browser(void *app);

    flecs::world m_world;
    std::string m_window_name;
    int m_window_width;
    int m_window_height;
    bool m_window_initialized{false};
};
