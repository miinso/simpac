#include "gameplay_module.h"
#include "components.h"
#include "pipelines.h"
#include "systems.h"

#include <modules/engine/core/components.h>
#include <modules/engine/physics/components.h>
#include <modules/engine/physics/pipelines.h>
#include <modules/engine/rendering/components.h>
#include <modules/engine/rendering/gui/components.h>
#include <modules/engine/rendering/gui/gui_module.h>

#include <raylib.h>
#include <raymath.h>

namespace gameplay {
    void GameplayModule::register_components(flecs::world world) { world.component<Spawner>(); }

    void GameplayModule::register_systems(flecs::world world) {
        m_spawner_tick = world.timer().interval(SPAWNER_INTERVAL);

        world.system<const Spawner, const core::GameSettings, const rendering::TrackingCamera>(
                     "Spawn enemy")
                .tick_source(m_spawner_tick)
                .term_at(1)
                .singleton()
                .term_at(2)
                .singleton()
                .each(systems::spawn_enemy);

        world.system<Cooldown>("Update cooldown")
                .without<CooldownCompleted>()
                .each(systems::update_cooldown);

        // when `CooldownCompleted` is removed from any entities,
        // run the following for each such event
        world.observer("Restart cooldown")
                .with<CooldownCompleted>()
                .event(flecs::OnRemove)
                .each(systems::restart_cooldown);

        world.system<core::Position, Attack, core::Speed, MultiProj *>("Fire projectile")
                .with<Projectile>() // this system runs on weapons like "dagger attack"
                .with<CooldownCompleted>()
                .write<CooldownCompleted>()
                .term_at(0)
                .parent() // weapon is equipped by a player, so get the `Position` from the parent
                          // entity
                .each(systems::fire_projectile);

        world.system("Handle collision for projectiles against wall, no-bounce")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .with<Projectile>()
                .without<Bounce>()
                .kind<OnCollisionDetected>()
                .immediate()
                .each(systems::handle_projectile_collision_against_wall);

        world.system("Handle collision for projectiles against, bounce")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .with<Projectile>()
                .kind<OnCollisionDetected>()
                .immediate()
                .each(systems::handle_projectile_collision_against_wall_bounce);

        world.system("Handle collision for projectiles, without any enchantments or modifiers")
                .with<Projectile>()
                .without<Pierce>()
                .without<Chain>()
                // .without<Split>()
                .with<physics::CollidedWith>(flecs::Wildcard)
                .kind<OnCollisionDetected>()
                .immediate()
                .each(systems::handle_projectile_collision);

        world.system<Pierce>("Handle collision for projectiles with 'pierce' modifier")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .kind<OnCollisionDetected>()
                .immediate()
                .each(systems::handle_projectile_collision_with_pierce);

        world.system<Chain, physics::Velocity, core::Position, rendering::Rotation, Attack>(
                     "Handle collision for projectiles with 'chain' modifier")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .kind<OnCollisionDetected>()
                .immediate() // this system runs on individual projectile
                .each(systems::handle_projectile_collision_with_chain);

        world.system<Split, physics::Velocity, core::Position, rendering::Rotation, Attack>(
                     "Handle collision for projectiles with 'split' modifier")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .kind<OnCollisionDetected>()
                .immediate()
                .each(systems::handle_projectile_collision_with_split);

        world.system<Damage>("Convert collision to damange")
                .with<physics::CollidedWith>(flecs::Wildcard)
                .write<TakeDamage>()
                .kind<OnCollisionDetected>()
                .each(systems::convert_collision_to_damage);

        world.system<Health, TakeDamage>("Process damage queue")
                .kind<PostCollisionDetected>()
                .each(systems::apply_damage);

        world.system<const Health>("Create health bar")
                .with<TakeDamage>()
                .without<rendering::ProgressBar>()
                .kind<PostCollisionDetected>()
                .each(systems::create_health_bar);

        world.system<const Health, rendering::ProgressBar>("Update health bar")
                .kind<PostCollisionDetected>()
                .each(systems::update_health_bar);

        world.system<Health, RegenHealth>("Apply health regen")
                .kind(flecs::OnUpdate) // this is the default phase. omitting `.kind()` makes it
                                       // `OnUpdate`
                .each(systems::apply_health_regen);

        add_multiproj =
                world.system("Enable multi projectiles")
                        .kind(0)
                        .with<Attack>()
                        .without<MultiProj>()
                        .with(flecs::ChildOf, "player") // not the same thing to a term modifier
                        .immediate()
                        .each(systems::enable_multi_projectiles);

        remove_multiproj = world.system("Disable multi projectiles")
                                   .kind(0)
                                   .with<Attack>()
                                   .with<MultiProj>()
                                   .with(flecs::ChildOf, "player")
                                   .immediate()
                                   .each(systems::disable_multi_projectiles);

        add_pierce =
                world.system("Add pierce enchant")
                        .kind(0)
                        .with<Projectile>()
                        .without<Pierce>()
                        .with(flecs::Prefab)
                        .immediate()
                        .each([world](flecs::entity e) { systems::add_pierce_enchant(world, e); });

        remove_pierce = world.system("Remove pierce enchant")
                                .kind(0)
                                .with<Projectile>()
                                .with<Pierce>()
                                .with(flecs::Prefab)
                                .immediate()
                                .each([world](flecs::entity e) {
                                    systems::remove_pierce_enchant(world, e);
                                });

        add_chain =
                world.system("Add chain enchant")
                        .kind(0)
                        .with<Projectile>()
                        .without<Chain>()
                        .with(flecs::Prefab)
                        .immediate()
                        .each([world](flecs::entity e) { systems::add_chain_enchant(world, e); });

        remove_chain = world.system("Remove chain enchant")
                               .kind(0)
                               .with<Projectile>()
                               .with<Chain>()
                               .with(flecs::Prefab)
                               .immediate()
                               .each([world](flecs::entity e) {
                                   systems::remove_chain_enchant(world, e);
                               });

        add_split =
                world.system("Add split enchant")
                        .kind(0)
                        .with<Projectile>()
                        .without<Split>()
                        .with(flecs::Prefab)
                        .immediate()
                        .each([world](flecs::entity e) { systems::add_split_enchant(world, e); });

        remove_split = world.system("Remove split enchant")
                               .kind(0)
                               .with<Projectile>()
                               .with<Split>()
                               .with(flecs::Prefab)
                               .immediate()
                               .each([world](flecs::entity e) {
                                   systems::remove_split_enchant(world, e);
                               });

        add_proj = world.system<MultiProj>("+1 proj")
                           .kind(0)
                           .with<Attack>()
                           .with(flecs::ChildOf, "player")
                           .each(systems::increase_projectile_count);

        remove_proj = world.system<MultiProj>("-1 proj")
                              .kind(0)
                              .with<Attack>()
                              .with(flecs::ChildOf, "player")
                              .each(systems::decrease_projectile_count);

        add_pierce_amt = world.system<Pierce>("+1 Pierce")
                                 .kind(0)
                                 .with<Projectile>()
                                 .with(flecs::Prefab)
                                 .each(systems::increase_pierce_count);

        remove_pierce_amt = world.system<Pierce>("-1 Pierce")
                                    .kind(0)
                                    .with<Projectile>()
                                    .with(flecs::Prefab)
                                    .each(systems::decrease_pierce_count);

        add_chain_amt = world.system<Chain>("+1 Chain")
                                .kind(0)
                                .with<Projectile>()
                                .with(flecs::Prefab)
                                .each(systems::increase_chain_count);

        remove_chain_amt = world.system<Chain>("-1 Chain")
                                   .kind(0)
                                   .with<Projectile>()
                                   .with(flecs::Prefab)
                                   .each(systems::decrease_chain_count);

        add_bounce = world.system("Add bounce enchant")
                             .kind(0)
                             .with<Projectile>()
                             .without<Bounce>()
                             .with(flecs::Prefab)
                             .immediate()
                             .each(systems::add_bounce_enchant);
        remove_bounce = world.system("Remove bounce enchant")
                                .kind(0)
                                .with<Projectile>()
                                .with<Bounce>()
                                .with(flecs::Prefab)
                                .immediate()
                                .each(systems::remove_bounce_enchant);

        add_bounce_amt = world.system<Bounce>("+1 Bounce")
                                 .kind(0)
                                 .with<Projectile>()
                                 .with(flecs::Prefab)
                                 .immediate()
                                 .each(systems::increase_bounce_count);

        remove_bounce_amt = world.system<Bounce>("-1 Bounce")
                                    .kind(0)
                                    .with<Projectile>()
                                    .with(flecs::Prefab)
                                    .immediate()
                                    .each(systems::decrease_bounce_count);
    }

    void GameplayModule::register_entities(flecs::world world) {
        auto dropdown = world.entity("gameplay_dropdown")
                                .child_of(rendering::gui::GUIModule::menu_bar)
                                .set<rendering::gui::MenuBarTab>({"Gameplay Tools", 25});

        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Add Multi Proj", add_multiproj, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Remove Multi Proj", remove_multiproj, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"+1 Proj", add_proj, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"-1 Proj", remove_proj, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Add Pierce", add_pierce, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Remove Pierce", remove_pierce, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"+1 Pierce", add_pierce_amt, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"-1 Pierce", remove_pierce_amt, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Add Chain", add_chain, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Remove Chain", remove_chain, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"+1 Chain", add_chain_amt, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"-1 Chain", remove_chain_amt, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Add Split", add_split, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Remove Split", remove_split, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Add Bounce", add_bounce, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"Remove Bounce", remove_bounce, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"+1 Bounce", add_bounce_amt, rendering::gui::MenuBarTabItemType::RUN});
        world.entity().child_of(dropdown).set<rendering::gui::MenuBarTabItem>(
                {"-1 Bounce", remove_bounce_amt, rendering::gui::MenuBarTabItemType::RUN});
    }

    void GameplayModule::register_pipeline(flecs::world world) {
        world.component<OnCollisionDetected>()
                .add(flecs::Phase)
                .depends_on<physics::CollisionDetection>();
        world.component<PostCollisionDetected>()
                .add(flecs::Phase)
                .depends_on<OnCollisionDetected>();
    }
} // namespace gameplay
