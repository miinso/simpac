// =========================================================================
// Cloth creation helpers
// =========================================================================
#include "components.h"
#include <flecs.h>

struct ClothConfig {
    int width = 10;      // number of particles in width
    int height = 10;     // number of particles in height
    Real spacing = 0.1;  // distance between particles
    Real mass = 1.0;     // mass per particle
    Real k_s = 1000.0;   // spring (weft, warp, diagonal) stiffness
    Real k_b = 100.0;    // spring bending stiffness
    Real k_d = 10.0;     // particle damping
    Vector3r offset = Vector3r::Zero();
};

inline void create_cloth(flecs::world& world, const ClothConfig& cfg) {
    // Create particles in grid
    std::vector<flecs::entity> particles;
    particles.resize(cfg.width * cfg.height);
    
    int particle_count = world.get_mut<Scene>().num_particles;
    
    for (int y = 0; y < cfg.height; y++) {
        for (int x = 0; x < cfg.width; x++) {
            int idx = y * cfg.width + x;
            
            Vector3r pos = cfg.offset + Vector3r(
                x * cfg.spacing,
                0.0,
                -y * cfg.spacing
            );
            
            particles[idx] = world.entity()
                .set(Position{pos})
                .set(OldPosition{pos})
                .set(Velocity{Vector3r::Zero()})
                .set(Acceleration{Vector3r::Zero()})
                .set(Mass{cfg.mass})
                .set(InverseMass{1.0 / cfg.mass})
                .set(ParticleIndex{particle_count++})
                .add<Particle>();

        }
    }
    
    world.get_mut<Scene>().num_particles = particle_count;
    
    // Helper to get particle at grid position
    auto get_particle = [&](int x, int y) -> flecs::entity {
        if (x < 0 || x >= cfg.width || y < 0 || y >= cfg.height)
            return flecs::entity::null();
        return particles[y * cfg.width + x];
    };
    
    // Create springs
    for (int y = 0; y < cfg.height; y++) {
        for (int x = 0; x < cfg.width; x++) {
            auto current = get_particle(x, y);
            
            // Structural springs (weft and warp)
            if (x < cfg.width - 1) {  // horizontal (weft)
                auto right = get_particle(x + 1, y);
                world.entity().set(Spring{
                    current, right, 
                    cfg.spacing, 
                    cfg.k_s, 
                    cfg.k_d
                });
            }
            
            if (y < cfg.height - 1) {  // vertical (warp)
                auto down = get_particle(x, y + 1);
                world.entity().set(Spring{
                    current, down,
                    cfg.spacing,
                    cfg.k_s,
                    cfg.k_d
                });
            }
            
            // Shear springs (diagonal)
            if (x < cfg.width - 1 && y < cfg.height - 1) {
                auto diagonal1 = get_particle(x + 1, y + 1);
                world.entity().set(Spring{
                    current, diagonal1,
                    cfg.spacing * std::sqrt(2.0),
                    cfg.k_s,
                    cfg.k_d
                });
                
                auto right = get_particle(x + 1, y);
                auto down = get_particle(x, y + 1);
                world.entity().set(Spring{
                    right, down,
                    cfg.spacing * std::sqrt(2.0),
                    cfg.k_s,
                    cfg.k_d
                });
            }
            
            // Bending springs (skip one particle)
            if (x < cfg.width - 2) {  // horizontal bending
                auto right2 = get_particle(x + 2, y);
                world.entity().set(Spring{
                    current, right2,
                    cfg.spacing * 2.0,
                    cfg.k_b,
                    cfg.k_d * 0.5
                });
            }
            
            if (y < cfg.height - 2) {  // vertical bending
                auto down2 = get_particle(x, y + 2);
                world.entity().set(Spring{
                    current, down2,
                    cfg.spacing * 2.0,
                    cfg.k_b,
                    cfg.k_d * 0.5
                });
            }
        }
    }
    
    // Pin top corners (optional)
    // get_particle(0, 0).set(InverseMass{0.0}).add<IsPinned>();
    // get_particle(cfg.width - 1, 0).set(InverseMass{0.0}).add<IsPinned>();
    for (int i = 0; i < cfg.width; ++i) {
        get_particle(i, 0)
            .add<IsPinned>();
    }
}