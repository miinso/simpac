// base_module.h
#pragma once

#include <iostream>
#include <ostream>

#include <flecs.h>

template<typename T>
struct BaseModule {
public:
    BaseModule(flecs::world &world) : m_world(world) {
        std::cout << "Creating Module " << typeid(T).name() << std::endl;
        world.module<T>();
        static_cast<T *>(this)->register_components(world);
        static_cast<T *>(this)->register_queries(world);
        static_cast<T *>(this)->register_systems(world);
        static_cast<T *>(this)->register_pipeline(world);
        static_cast<T *>(this)->register_submodules(world);
        static_cast<T *>(this)->register_entities(world);
    }

    ~BaseModule() = default;

    void print() { std::cout << "Base" << std::endl; }
    virtual flecs::world get_world() const { return m_world; }

protected:
    flecs::world m_world;

private:
    BaseModule() = delete;

    void register_components(flecs::world &world) {
        std::cout << "No pipeline registration implemented" << std::endl;
    }

    void register_systems(flecs::world &world) {
        std::cout << "No pipeline registration implemented" << std::endl;
    }

    void register_pipeline(flecs::world &world) {
        std::cout << "No pipeline registration implemented" << std::endl;
    }

    void register_submodules(flecs::world &world) {
        std::cout << "No submodule registration implemented" << std::endl;
    }

    void register_entities(flecs::world &world) {
        std::cout << "No entity registration implemented" << std::endl;
    }

    void register_queries(flecs::world &world) {
        std::cout << "No query registration implemented" << std::endl;
    }
};
