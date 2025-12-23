// components/physics.hpp
// collate common components
namespace components {
    struct Position { Vector3r value; };
    struct Velocity { Vector3r value; };
    struct Force { Vector3r value; };
    struct Mass { float value; };
    // ... 기타 기본 컴포넌트들
}

// components/collider.hpp 
namespace components {
    struct CollisionShape {
        enum class Type { Box, Sphere, Mesh };
        Type type;
        std::unique_ptr<Discregrid::CubicLagrangeDiscreteGrid> sdf;
        Vector3r scale{1.0f, 1.0f, 1.0f};
        
        // 팩토리 메서드로 생성을 단순화
        static CollisionShape CreateFromMesh(const Mesh& mesh);
        static CollisionShape CreateBox(const Vector3r& size);
    };

    struct Contact {
        EntityId entity1;
        EntityId entity2;
        Vector3r point;
        Vector3r normal;
        float depth;
    };
}

// systems/physics_pipeline.hpp
// some pipeline maybe
namespace systems {
    class PhysicsPipeline {
    public:
        static void Configure(flecs::world& ecs) {
            // registration order is the execution order
            ecs.system<Position, Velocity, Force>("IntegrateForces")
                .kind(PreUpdate)
                .each(IntegrateForces);
                
            ecs.system<Position, CollisionShape>("BroadPhase")
                .kind(OnUpdate)
                .each(BroadPhaseCollision);
                
            // ... some more
        }
        
    private:
        static void IntegrateForces(Position& pos, Velocity& vel, const Force& force);
        static void BroadPhaseCollision(Position& pos, const CollisionShape& shape);
    };
}

// resources/resource_manager.hpp
// use raylib (DRY)
namespace resources {
    class ResourceManager {
    public:
        static void LoadMesh(const std::string& name, const std::string& path);
        static void LoadCollisionShape(const std::string& name, const CollisionShape& shape);
        
        static const Mesh& GetMesh(const std::string& name);
        static const CollisionShape& GetCollisionShape(const std::string& name);
    };
}

// prefabs/physics_prefabs.hpp
// reusables
namespace prefabs {
    flecs::entity CreateRigidBody(flecs::world& ecs,
                                 const Vector3r& position,
                                 const std::string& mesh_name,
                                 float mass) {
        auto entity = ecs.entity()
            .add<components::Position>({position})
            .add<components::Velocity>()
            .add<components::Mass>({mass})
            .add<components::CollisionShape>(
                resources::ResourceManager::GetCollisionShape(mesh_name));
            
        return entity;
    }
}

// main.cpp
int main() {
    flecs::world ecs;
    
    systems::PhysicsPipeline::Configure(ecs);
    
    resources::ResourceManager::LoadMesh("cube", "assets/cube.obj");
    resources::ResourceManager::LoadCollisionShape("cube_collision",
        components::CollisionShape::CreateBox({1.0f, 1.0f, 1.0f}));
    
    auto ground = prefabs::CreateRigidBody(ecs, {0, -5, 0}, "cube", 0.0f);
    ground.add<components::Static>();
    
    auto dynamic_box = prefabs::CreateRigidBody(ecs, {0, 5, 0}, "cube", 1.0f);
    
    while (!shouldClose()) {
        ecs.progress();
        render();
    }
}