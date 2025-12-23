#pragma once

#include <cstdio>
#include <flecs.h>
#include <typeinfo>

template<typename T>
struct BaseModule {
public:
    BaseModule(flecs::world &world) : m_world(world) {
        std::printf("Creating Module %s", typeid(T).name());
        world.module<T>();
        static_cast<T *>(this)->register_components(world);
        static_cast<T *>(this)->register_queries(world);
        static_cast<T *>(this)->register_systems(world);
        static_cast<T *>(this)->register_pipeline(world);
        static_cast<T *>(this)->register_submodules(world);
        static_cast<T *>(this)->register_entities(world);
    }
    ~BaseModule() = default;

    void print() { std::printf("Base\n"); }
    virtual flecs::world get_world() const { return m_world; }

protected:
    flecs::world m_world;

private:
    BaseModule() = delete;

    void register_components(flecs::world &world) {
        std::printf("No pipeline registration implemented\n");
    }

    void register_systems(flecs::world &world) {
        std::printf("No pipeline registration implemented\n");
    }

    void register_pipeline(flecs::world &world) {
        std::printf("No pipeline registration implemented\n");
    }

    void register_submodules(flecs::world &world) {
        std::printf("No submodule registration implemented\n");
    }

    void register_entities(flecs::world &world) {
        std::printf("No entity registration implemented\n");
    }

    void register_queries(flecs::world &world) {
        std::printf("No query registration implemented\n");
    }
};
