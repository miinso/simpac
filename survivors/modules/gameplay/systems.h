#pragma once

#include "components.h"

#include <modules/engine/core/components.h>
#include <modules/engine/core/queries.h>
#include <modules/engine/core/systems.h>
#include <modules/engine/physics/components.h>
#include <modules/engine/rendering/components.h>

#include <raylib.h>
#include <raymath.h>

namespace gameplay {
    namespace systems {
        inline bool &outside_side_switch() {
            static bool outside_side_switch = false;
            return outside_side_switch;
        } // i don't like this. i need something better whilst using cpp11

        inline void spawn_enemy(flecs::iter &it, size_t i, const Spawner &spawner,
                                const core::GameSettings &settings) {
            float factor = rand() % 2 - 1;
            float neg = rand() % 1 - 1;
            float randX = outside_side_switch() ? neg * factor * settings.windowWidth
                                                : rand() % settings.windowWidth;
            float randY = outside_side_switch() ? rand() % settings.windowHeight
                                                : neg * factor * settings.windowHeight;

            it.world()
                    .entity()
                    .is_a(spawner.enemy_prefab)
                    .child_of(it.entity(i))
                    .set<core::Position>({Eigen::Vector3f(randX, randY, 0)});

            outside_side_switch() = !outside_side_switch();
        };

        inline void update_cooldown(flecs::iter &it, size_t i, Cooldown &cooldown) {
            cooldown.elapsed_time += it.delta_time();
            if (cooldown.elapsed_time > cooldown.value) {
                it.entity(i).add<CooldownCompleted>();
            }
        };

        inline void restart_cooldown(flecs::entity e) {
            if (e.has<Cooldown>()) {
                e.set<Cooldown>({e.get<Cooldown>().value, 0.0f});
            }
        };

        inline void fire_projectile(flecs::iter &it, size_t index, core::Position &pos,
                                    Attack &attack, core::Speed &speed, MultiProj *multi_proj) {
            auto weapon = it.entity(index); // "dagger attack"

            float shortest_distance_sqr = 1e6;
            core::Position target_pos{pos.value};

            core::queries::position_and_tag().each(
                    [&](flecs::entity other, core::Position &other_pos, core::Tag &tag) {
                        if (attack.target_tag != tag.name)
                            return;
                        float d = (pos.value - other_pos.value).squaredNorm();
                        if (d > shortest_distance_sqr)
                            return;
                        shortest_distance_sqr = d;
                        target_pos = other_pos;
                    });

            if (target_pos.value == pos.value)
                return; // prevent singularity

            Eigen::Vector2f direction = (pos.value - target_pos.value).head(2).normalized();
            float rot =
                    Vector2Angle(Vector2{0, 1}, Vector2{direction.x(), direction.y()}) * RAD2DEG;

            auto velocity = (target_pos.value - pos.value).normalized() * speed.value;

            int proj_count = multi_proj ? multi_proj->projectile_count : 1;
            float spread_angle = multi_proj ? multi_proj->spread_angle : 0.0f;

            float offset = proj_count % 2 == 0 ? spread_angle / proj_count / 2 : 0;
            auto prefab = it.world().lookup(attack.attack_prefab_name.c_str());

            for (int i = -proj_count / 2; i < (proj_count + 1) / 2; ++i) {
                auto angle_offset = (i * (spread_angle / proj_count) + offset);
                Eigen::AngleAxisf rotation(angle_offset * DEG2RAD, Eigen::Vector3f::UnitZ());
                it.world()
                        .entity()
                        .is_a(prefab)
                        .child_of(weapon)
                        .set<core::Position>({pos.value})
                        .set<rendering::Rotation>({rot + angle_offset})
                        .set<core::Speed>({150})
                        .set<physics::Velocity>({rotation * velocity});
            }
            weapon.remove<CooldownCompleted>();
        };

        inline void handle_projectile_collision(flecs::entity e) {
            e.add<core::DestroyAfterFrame>();
        };

        inline void handle_projectile_collision_with_pierce(flecs::iter &it, size_t i,
                                                            Pierce &pierce) {
            auto other = it.pair(1).second();
            if (pierce.hits.count(other.id())) { // `.count` tells you if an element exists
                it.entity(i).remove<physics::CollidedWith>(other);
                return;
            }

            pierce.hits.insert(other.id());
            pierce.pierce_count -= 1;

            if (pierce.pierce_count < 0) {
                it.entity(i).add<core::DestroyAfterFrame>();
            }
        };

        inline void handle_projectile_collision_with_chain(flecs::iter &it, size_t i, Chain &chain,
                                                           physics::Velocity &vel,
                                                           core::Position &pos,
                                                           rendering::Rotation &rot,
                                                           Attack &attack) {
            auto other = it.pair(5).second();

            if (chain.hits.count(other.id())) {
                // a projectile interact only once
                it.entity(i).remove<physics::CollidedWith>(other);
                return;
            }

            chain.hits.insert(other.id());
            chain.chain_count -= 1;

            float shortest_distance_sqr = 1e6;
            core::Position target_pos{pos.value};

            core::queries::position_and_tag().each(
                    [&](flecs::entity collidable, core::Position &collidable_pos, core::Tag &tag) {
                        if (!chain.hits.count(collidable.id()) && attack.target_tag == tag.name &&
                            other.id() != collidable.id()) {
                            float d = (pos.value - collidable_pos.value).squaredNorm();
                            if (d > shortest_distance_sqr)
                                return;
                            shortest_distance_sqr = d;
                            target_pos = collidable_pos;
                        }
                    });

            if (target_pos.value == pos.value)
                return; // prevent singularity

            Eigen::Vector3f direction = (pos.value - target_pos.value).normalized();
            rot.angle =
                    Vector2Angle(Vector2{0, 1}, Vector2{direction.x(), direction.y()}) * RAD2DEG;

            auto new_velocity = -direction * vel.value.norm();
            vel.value = new_velocity;
        };

        inline void handle_projectile_collision_with_split(flecs::iter &it, size_t i, Split &split,
                                                           physics::Velocity &vel,
                                                           core::Position &pos,
                                                           rendering::Rotation &rot,
                                                           Attack &attack) {
            auto other = it.pair(5).second(); // `other` is what hit by the projectile
            if (split.hits.count(other.id())) {
                // a projectile interact only once
                it.entity(i).remove<physics::CollidedWith>(other);
                return;
            }

            split.hits.insert(other.id());

            auto new_velocity_left = Eigen::AngleAxisf(-90.0f * DEG2RAD, Eigen::Vector3f::UnitZ()) *
                                     vel.value; // right-hand rule
            auto new_velocity_right =
                    Eigen::AngleAxisf(90.0f * DEG2RAD, Eigen::Vector3f::UnitZ()) * vel.value;

            auto prefab = it.world().lookup(attack.attack_prefab_name.c_str());
            it.world()
                    .entity()
                    .is_a(prefab)
                    .set<core::Position>({pos})
                    .set<rendering::Rotation>({rot.angle - 90.0f})
                    .set<physics::Velocity>({new_velocity_left})
                    .remove<Split>() // remove enchantments bc it's too op
                    .remove<Chain>()
                    .remove<Pierce>();

            it.world()
                    .entity()
                    .is_a(prefab)
                    .set<core::Position>({pos})
                    .set<rendering::Rotation>({rot.angle + 90.0f})
                    .set<physics::Velocity>({new_velocity_right})
                    .remove<Split>() // remove enchantments bc it's too op
                    .remove<Chain>()
                    .remove<Pierce>();
        };

        inline void convert_collision_to_damage(flecs::iter &it, size_t i, Damage &damage) {
            auto other = it.pair(1).second();
            if (other.has<TakeDamage>() && !other.has<Health>())
                return;
            other.set<TakeDamage>({damage.value});
        };

        inline void apply_damage(flecs::entity e, Health &health, TakeDamage &take_damage) {
            health.value -= take_damage.damage;
            if (health.value <= 0) {
                e.add<core::DestroyAfterFrame>();
            }
            e.remove<TakeDamage>();
        };

        inline void create_health_bar(flecs::entity e, const Health &hp) {
            e.set<rendering::ProgressBar>({0, hp.max, hp.value, 0, 0, 50, 10});
        };

        inline void apply_health_regen(flecs::iter &it, size_t, Health &health,
                                       RegenHealth &regen_health) {
            health.value = std::min(health.value + regen_health.rate * it.delta_time(), health.max);
        };

        inline void update_health_bar(const Health &health, rendering::ProgressBar &bar) {
            bar.current_val = health.value;
        }

        // callables
        inline void enable_multi_projectiles(flecs::entity e) {
            e.set<MultiProj>({2, 30, 150, 30});
        }

        inline void disable_multi_projectiles(flecs::entity e) { e.remove<MultiProj>(); }

        inline void add_pierce_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Chain>();
            e.set<Pierce>({1, std::unordered_set<int>()});
            core::systems::remove_empty_tables(world);
        }

        inline void remove_pierce_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Pierce>();
            core::systems::remove_empty_tables(world);
        }

        inline void add_chain_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Pierce>();
            e.set<Chain>({1, std::unordered_set<int>()});
            core::systems::remove_empty_tables(world);
        }

        inline void remove_chain_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Chain>();
            core::systems::remove_empty_tables(world);
        }

        inline void add_split_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Pierce>();
            e.remove<Chain>();
            e.set<Split>({std::unordered_set<int>()});
            core::systems::remove_empty_tables(world);
        }

        inline void remove_split_enchant(const flecs::world &world, flecs::entity e) {
            e.remove<Split>();
            core::systems::remove_empty_tables(world);
        }

        inline void increase_projectile_count(MultiProj &multi) {
            multi.projectile_count++;
            multi.spread_angle = std::min(multi.spread_angle + 15.0f, multi.max_spread);
        }

        inline void decrease_projectile_count(MultiProj &multi) {
            multi.projectile_count = std::max(multi.projectile_count - 1, 1);
            multi.spread_angle = std::max(multi.spread_angle - 15.0f, multi.min_spread);
        }

        inline void increase_pierce_count(Pierce &pierce) { pierce.pierce_count++; }

        inline void decrease_pierce_count(Pierce &pierce) {
            pierce.pierce_count = std::min(0, pierce.pierce_count - 1);
        }

        inline void increase_chain_count(Chain &chain) { chain.chain_count++; }

        inline void decrease_chain_count(Chain &chain) {
            chain.chain_count = std::min(0, chain.chain_count - 1);
        }
    } // namespace systems
} // namespace gameplay
