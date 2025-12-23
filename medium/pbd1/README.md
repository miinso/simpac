# picture

1. each constraint has its own struct
```cpp
struct DistanceConstraint {
    flecs::entity e1, e2;
    float rest_length;
    float lambda;
    
    void solve() {
        auto x1 = e1.get_mut<Position>();
        auto x2 = e2.get_mut<Position>();
        
    }
};
```

```cpp
// 기본 컨스트레인트 인터페이스
struct IConstraint {
    virtual void solve(float dt) = 0;
    virtual void project() = 0;
    
    float stiffness;
    float lambda;
};

// 거리 컨스트레인트 구현
struct DistanceConstraint : IConstraint {
    void solve(float dt) override {
        // 현재 구현처럼 거리 제약 해결
    }
    
    void project() override {
        // 위치 투영
    }
    
    flecs::entity particles[2];
    float rest_length;
};
```

2. treat particles as individual entities

파티클은 어케 잡으면 좋을까? 이게 생각이 의외로 잘 안되네???

Particle
    Position
    OldPosition
    Velocity

DistanceConstraint
    flecs::entity e1
    flecs::entity e2
    Stiffness
    Lambda


ecs.system<Particle>().each(particle_integrate);
ecs.system<Particle>().each(velocity_update);
ecs.system<Particle>().each(particle_integrate);

프리팹이나 릴레이션 그런걸 쓸때가 왔나?

컨스트레인트 타입들:
Distance
Bending
Area
Volume
Fem


콘스트레인트 솔브 루프 관련 접근이 여럿 있겠는데?
1.
ecs.system<DistanceConstraint>().each(solve_distance_constraint);

2.
ecs.system<Constraint>().with<DistanceType>().each(solve_constraint or solve_distance_constraint);
ecs.system<Constraint>().with<Bending>().each(solve_constraint or solve_distance_constraint);

3. 

# system

Step
    Clear Acceleration (set a to zero, then apply gravity)
    Substep
        Integrate Velocity
        ~~Damp Velocity~~
        Integrate Position
        Clear Constraint Lambda
        Solver Loop
            Position Constraint Solve // collision constraints are also solved here
        Update Velocity
    (update mesh for rb)
    Clear Contacts
    Collision Detection (for ccd, test vectors of x->p)
    Register Collision Constraints
    Velocity Constraints Solve (Contact, Friction and Restitution)

simplified step:
```
Step
    for all particles: Clear Acceleration (set a to zero, then apply gravity)
    Substep
        for all particles: Integrate
        for all constraints: Clear Constraint Lambda
        Solver Loop
            for all constraints: Position Constraint Solve // collision constraints are also solved here
        for all particles: Update Velocity
```


where does "Collision Detect" go?
contacts constraints are solved as velocity contraints

ecs.system<Acceleration>().with<Particle>().each(pbd_clear_acceleration);

ecs.system<Posiiton, OldPosition, Mass, LinearVelocity, Acceleration>().with<Particle>().each(pbd_particle_integrate);