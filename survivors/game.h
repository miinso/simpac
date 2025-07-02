#pragma once

#include <flecs.h>

#include <string>

struct Game {
public:
    Game(const char *windowName, int windowWidth, int windowHeight);
    void run();

private:
    flecs::world m_world;
    std::string m_windowName;
    int m_windowWidth;
    int m_windowHeight;
};
