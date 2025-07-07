#pragma once

#include <modules/base_module.h>

namespace gameplay {
    constexpr float SPAWNER_INTERVAL = 0.05f;

    struct GameplayModule : public BaseModule<GameplayModule> {
    public:
        GameplayModule(flecs::world &world) : BaseModule(world){};

    private:
        flecs::entity m_spawner_tick;

        flecs::system add_multiproj;
        flecs::system remove_multiproj;
        flecs::system add_pierce;
        flecs::system remove_pierce;
        flecs::system add_chain;
        flecs::system remove_chain;
        flecs::system add_split;
        flecs::system remove_split;
        flecs::system add_proj;
        flecs::system remove_proj;
        flecs::system add_pierce_amt;
        flecs::system remove_pierce_amt;
        flecs::system add_chain_amt;
        flecs::system remove_chain_amt;
        flecs::system add_bounce;
        flecs::system remove_bounce;
        flecs::system add_bounce_amt;
        flecs::system remove_bounce_amt;

        void register_components(flecs::world);
        void register_systems(flecs::world);
        void register_pipeline(flecs::world);
        void register_entities(flecs::world);

        friend struct BaseModule<GameplayModule>;
    };
} // namespace gameplay
