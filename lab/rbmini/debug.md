// debug/types.hpp
#pragma once
#include "core/types.hpp"
#include <vector>
#include <string>

namespace phys::debug {
    struct DebugPoint {
        Vector3r position;
        Vector3r color;
        Real size;
        std::string label;
    };

    struct DebugLine {
        Vector3r start;
        Vector3r end;
        Vector3r color;
        Real thickness;
    };

    struct DebugArrow {
        Vector3r start;
        Vector3r direction;
        Vector3r color;
        Real length;
        Real thickness;
    };

    struct DebugText {
        Vector3r position;
        std::string text;
        Vector3r color;
        Real scale;
        bool world_space;
    };

    struct FrameDebugData {
        std::vector<DebugPoint> points;
        std::vector<DebugLine> lines;
        std::vector<DebugArrow> arrows;
        std::vector<DebugText> texts;
        
        void clear() {
            points.clear();
            lines.clear();
            arrows.clear();
            texts.clear();
        }
    };
}

// debug/components.hpp
#pragma once
#include "core/types.hpp"

namespace phys::components {
    struct DebugSettings {
        bool show_aabbs = false;
        bool show_collision_points = false;
        bool show_constraints = false;
        bool show_forces = false;
        bool show_velocities = false;
        bool show_contact_normals = false;
        bool show_grid = false;
        Vector3r debug_color = {0.0f, 1.0f, 0.0f};
    };

    struct PerformanceStats {
        int frame_count = 0;
        float frame_time = 0.0f;
        float physics_time = 0.0f;
        float collision_time = 0.0f;
        float constraint_time = 0.0f;
        int collision_checks = 0;
        int active_constraints = 0;
    };
}

// debug/draw.hpp
#pragma once
#include "debug/types.hpp"

namespace phys::debug {
    namespace draw {
        inline void point(FrameDebugData& data,
                         const Vector3r& position,
                         const Vector3r& color = Vector3r(0,1,0),
                         Real size = 0.1f,
                         const std::string& label = "") {
            data.points.push_back({position, color, size, label});
        }

        inline void line(FrameDebugData& data,
                        const Vector3r& start,
                        const Vector3r& end,
                        const Vector3r& color = Vector3r(0,1,0),
                        Real thickness = 1.0f) {
            data.lines.push_back({start, end, color, thickness});
        }

        inline void arrow(FrameDebugData& data,
                         const Vector3r& start,
                         const Vector3r& direction,
                         const Vector3r& color = Vector3r(0,1,0),
                         Real length = 1.0f,
                         Real thickness = 1.0f) {
            data.arrows.push_back({start, direction, color, length, thickness});
        }

        inline void text(FrameDebugData& data,
                        const Vector3r& position,
                        const std::string& text,
                        const Vector3r& color = Vector3r(0,1,0),
                        Real scale = 1.0f,
                        bool world_space = true) {
            data.texts.push_back({position, text, color, scale, world_space});
        }

        inline void aabb(FrameDebugData& data,
                        const Vector3r& min,
                        const Vector3r& max,
                        const Vector3r& color = Vector3r(0,0,1)) {
            const Vector3r corners[8] = {
                Vector3r(min.x(), min.y(), min.z()),
                Vector3r(max.x(), min.y(), min.z()),
                Vector3r(min.x(), max.y(), min.z()),
                Vector3r(max.x(), max.y(), min.z()),
                Vector3r(min.x(), min.y(), max.z()),
                Vector3r(max.x(), min.y(), max.z()),
                Vector3r(min.x(), max.y(), max.z()),
                Vector3r(max.x(), max.y(), max.z())
            };

            line(data, corners[0], corners[1], color);
            line(data, corners[1], corners[3], color);
            line(data, corners[3], corners[2], color);
            line(data, corners[2], corners[0], color);

            line(data, corners[4], corners[5], color);
            line(data, corners[5], corners[7], color);
            line(data, corners[7], corners[6], color);
            line(data, corners[6], corners[4], color);

            line(data, corners[0], corners[4], color);
            line(data, corners[1], corners[5], color);
            line(data, corners[2], corners[6], color);
            line(data, corners[3], corners[7], color);
        }

        inline void coordinate_system(FrameDebugData& data,
                                    const Vector3r& position,
                                    const Quaternionr& orientation,
                                    Real scale = 1.0f) {
            Matrix3r R = orientation.matrix();
            arrow(data, position, R.col(0), Vector3r(1,0,0), scale); // X축
            arrow(data, position, R.col(1), Vector3r(0,1,0), scale); // Y축
            arrow(data, position, R.col(2), Vector3r(0,0,1), scale); // Z축
        }
    }
}

// systems/debug.hpp
#pragma once
#include "debug/draw.hpp"
#include <flecs.h>
#include <chrono>

namespace phys::systems {
    void collect_debug_info(flecs::iter& it,
                          const components::DebugSettings* settings,
                          components::PerformanceStats* stats) {
        static auto last_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        for (auto i : it) {
            stats[i].frame_count++;
            stats[i].frame_time = dt;
            
            // collect collision info
        }
    }

    void visualize_aabbs(flecs::iter& it,
                        debug::FrameDebugData& debug_data,
                        const components::DebugSettings* settings,
                        const components::BoundingVolume* volumes) {
        for (auto i : it) {
            if (settings[i].show_aabbs) {
                const auto& aabb = volumes[i].aabb;
                debug::draw::aabb(debug_data, aabb.min, aabb.max);
            }
        }
    }

    void visualize_constraints(flecs::iter& it,
                             debug::FrameDebugData& debug_data,
                             const components::DebugSettings* settings) {
        if (!settings->show_constraints) return;

        it.world().each<components::DistanceConstraint>(
            [&](const components::DistanceConstraint& constraint) {
                auto e1 = constraint.entity1;
                auto e2 = constraint.entity2;
                
                if (!e1.is_alive() || !e2.is_alive()) return;
                
                auto pos1 = e1.get<components::Position>();
                auto pos2 = e2.get<components::Position>();
                
                debug::draw::line(debug_data,
                                pos1->value,
                                pos2->value,
                                Vector3r(1,1,0));  // 노란색
                
                Vector3r mid = (pos1->value + pos2->value) * 0.5f;
                float dist = (pos2->value - pos1->value).norm();
                debug::draw::text(debug_data,
                                mid,
                                "Dist: " + std::to_string(dist));
            });
    }

    void visualize_contacts(flecs::iter& it,
                          debug::FrameDebugData& debug_data,
                          const components::DebugSettings* settings) {
        if (!settings->show_collision_points) return;

        it.world().each<collision::ContactConstraint>(
            [&](const collision::ContactConstraint& contact) {
                const auto& point = contact.contact;
                
                debug::draw::point(debug_data,
                                 point.position,
                                 Vector3r(1,0,0)); 
                
                if (settings->show_contact_normals) {
                    debug::draw::arrow(debug_data,
                                     point.position,
                                     point.normal,
                                     Vector3r(0,0,1),
                                     0.2f);
                }
            });
    }

    void display_performance_stats(flecs::iter& it,
                                 debug::FrameDebugData& debug_data,
                                 const components::PerformanceStats* stats) {
        for (auto i : it) {
            const auto& stat = stats[i];
            
            std::string info = 
                "FPS: " + std::to_string(1.0f / stat.frame_time) + "\n"
                "Physics Time: " + std::to_string(stat.physics_time * 1000.0f) + "ms\n"
                "Collision Checks: " + std::to_string(stat.collision_checks) + "\n"
                "Active Constraints: " + std::to_string(stat.active_constraints);
            
            debug::draw::text(debug_data,
                            Vector3r(10, 10, 0),  
                            info,
                            Vector3r(1,1,1),
                            1.0f,
                            false);  
        }
    }

    void register_debug_systems(flecs::world& ecs) {
        ecs.system<const components::DebugSettings,
                  components::PerformanceStats>()
            .kind(flecs::PreUpdate)
            .iter(collect_debug_info);

        ecs.system<debug::FrameDebugData,
                  const components::DebugSettings,
                  const components::BoundingVolume>()
            .kind(flecs::PostUpdate)
            .iter(visualize_aabbs);

        ecs.system<debug::FrameDebugData,
                  const components::DebugSettings>()
            .kind(flecs::PostUpdate)
            .iter(visualize_constraints);

        ecs.system<debug::FrameDebugData,
                  const components::DebugSettings>()
            .kind(flecs::PostUpdate)
            .iter(visualize_contacts);

        ecs.system<debug::FrameDebugData,
                  const components::PerformanceStats>()
            .kind(flecs::PostUpdate)
            .iter(display_performance_stats);
    }
}