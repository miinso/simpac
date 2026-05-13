#pragma once

#include "../particle/particle.h"
#include <vector>

struct ParticleInteractionState {
    flecs::entity hovered;
    flecs::entity selected;
    flecs::entity pressed;

    bool pointer_down = false;
    bool pointer_pressed = false;
    bool pointer_released = false;
    bool dragging = false;

    float press_x = 0.0f;
    float press_y = 0.0f;
    float last_x = 0.0f;
    float last_y = 0.0f;

    vec3f drag_plane_point = {0.0f, 0.0f, 0.0f};
    vec3f drag_plane_normal = {0.0f, 1.0f, 0.0f};
    vec3f drag_offset = {0.0f, 0.0f, 0.0f};
    bool drag_added_pin = false;

    float virtual_spring_k = 800.0f;
    float virtual_spring_d = 24.0f;

    float drag_threshold_px = 3.0f;
    float pick_radius_scale = 1.2f;
};

struct TriangleInteractionState {
    flecs::entity hovered;
    flecs::entity selected;
    flecs::entity pressed;

    bool pointer_down = false;
    bool pointer_pressed = false;
    bool pointer_released = false;
    bool dragging = false;

    float press_x = 0.0f;
    float press_y = 0.0f;
    float last_x = 0.0f;
    float last_y = 0.0f;

    vec3f drag_plane_point = {0.0f, 0.0f, 0.0f};
    vec3f drag_plane_normal = {0.0f, 1.0f, 0.0f};
    vec3f drag_offset = {0.0f, 0.0f, 0.0f};

    vec3f drag_vertex_offsets[3] = {};
    bool drag_added_pins = false;

    float virtual_spring_k = 800.0f;
    float virtual_spring_d = 24.0f;
    float drag_threshold_px = 3.0f;
};
