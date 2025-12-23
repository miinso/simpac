#pragma once

#include <flecs.h>
#include <unordered_set>

namespace gameplay {
    struct Spawner {
        flecs::entity enemy_prefab;
    };

    struct TakeDamage {
        float damage;
    };

    struct Damage {
        float value;
    };

    struct Health {
        float max;
        float value;
    };

    struct Attack {
        std::string attack_prefab_name;
        std::string target_tag;
    };

    struct RegenHealth {
        float rate;
    };

    struct Cooldown {
        float value;
        float elapsed_time;
    };

    struct CooldownCompleted {};

    struct Projectile {};

    struct Bounce {
        int bounce_count;
    };

    struct Pierce {
        int pierce_count;
        std::unordered_set<int> hits;
    };

    struct Chain {
        int chain_count;
        std::unordered_set<int> hits;
    };

    struct Split {
        std::unordered_set<int> hits;
    };

    struct MultiProj {
        int projectile_count;
        float spread_angle;
        float max_spread;
        float min_spread;
    };
} // namespace gameplay
