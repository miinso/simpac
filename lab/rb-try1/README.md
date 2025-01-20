# ex_flecs

kdtree exercise.

# entities

- Position

- Mesh

- CollisionObject
- KdTree
- AABB
- Geometry
  - IndexedMesh

# hierarchy

- Model

  - RigidBody
  - TriangleModel
  - TetModel

- CollisionObject (tag)
  - Bvh
  - AABB
  - Geometry
    - Mesh
  - Collider (sdf only for now)
    - Discregrid::CubicLagrangeDiscreteGrid sdf;
    - void collision_test(x, tol, &cp, &n, &dist);
    - void distance(x, tol);

```cpp
auto collision_object = ecs.entity()
    .add<ParticleBVH>()
    .add<AABB>();

collision_object.add<CollisionObject>();
```

# systems

- 콜리전 페어 수집. 단순 n\*(n-1)/2 페어 > 가 아니고, 그냥 i == j 인 경우 빼고 다 중복으로 수집함. 버텍스-sdf 콜리전을 하는거같음
- 콜리전오브젝트의 aabb 및 bvh 업데이트
  - 파티클 레인지, mesh 오브젝트, 버텍스 갯수
- 모든 콜리전 페어 대상으로:

  - aabb 겹치는지 일단 체크함. 안겹치면 패스
  - rb1, co1, rb2, co2 준비해서 둘 사이 디텍션 수행함 (co가 rb 인덱스를 담고있어서, 모델에서 rb 찾을 수 있음)
    - rb1의 bvh 에 dfs(predicate, cb) 돌리는데, predicate 이 broad를 한번 더하는 것 같음. 미디움 페이즈 느낌? callback에서 - - 내로우페이즈 함. leaf 일때까지 내려가서 (최대 엘레먼트 10개)
    - predicate 에서는:
      - 확정적으로 침투가 있는 경우에 true 를 반환함. (center of bounding sphere intersects AABB)
      - 좀 특별한 aabb 체크, 그리고 co->distance() sdf 쿼리를 함 날려서 브로드보다 좀 자세히 검사함
    - bvh 에 dfs 순회 하면서 predicate 만족하는 노드들은, 콜백에서:
      - 확정적으로 rb2와 침투가 있는 rb1 bvh 의 leaf 노드에 담긴 모든 버텍스들을 가지고, rb2의 sdf 대상으로 충돌 테스트 수행함.
      - rb1의 각 버텍스들을 rb2의 바디스페이스로 변환시켜서 테스트 돌림. 그럼 적어도 한개는 콘택트 케이스가 나옴.
      - 콘택트 정보는 튜플 같은데, (rb1의 버텍스 월드 좌표, cp? 콘택트포인트 또는 closest point, 월드 콘택트 노말, distance) 이렇게 나옴.
    - 콜백에서, 글로벌 컨테이너에 컨택트 푸시 함.
  - 수집한 콘택트들, 모델 오브젝트? 에 등록함

  m_contactCB(RigidBodyContactType, rbIndex1, rbIndex2, cp1, cp2, normal, dist, restitutionCoeff, frictionCoeff, m_contactCBUserData);

  - cb를 모델에 등록해놓으면, 컨택트 생길때마다 그 cb이 실행됨. rb-rb contact 의 경우에는:
    - 이 콜백은 setContactCallback 로 등록함.
    - contactCallbackFunction 은
    - 결과적으로 model->addRigidBodyContactConstraint(bodyIndex1, bodyIndex2, cp1, cp2, normal, dist, restitutionCoeff, frictionCoeff);
    - 구에엑 oop hell..

```cpp
m_rigidBodyContactConstraints.emplace_back(RigidBodyContactConstraint());
RigidBodyContactConstraint &cc = m_rigidBodyContactConstraints.back();
const bool res = cc.initConstraint(*this, rbIndex1, rbIndex2, cp1, cp2, normal, dist, restitutionCoeff, m_contactStiffnessRigidBody, frictionCoeff);
if (!res)
    m_rigidBodyContactConstraints.pop_back();
return res;
```

- float co::distance(x, tol)
  - 이거는 그냥 sdf 쿼리인거임.
- bool co::collisionTest(x, tol, &cp, &n, &dist)
  - 이거는 distanceEx 임. 같은 sdf 쿼리인데, 추가로 이것저것 반환함

## add contact constraint

```cpp
struct ContactConstraint {};

// add pbd contact constraint
ecs.system<const Contact>().each([](const Contact& contact) {
    ecs.entity().set<ContactConstraint>({rb1, rb2});
});
```


## broadphase psuedo

```cpp
auto q1 = ecs.query_builder<RigidBody>().build();
auto q2 = ecs.query_builder<AABB>().with<RigidBody>().build();
auto q3 = ecs.query_builder<AABB, BVH, CollisionObject>().with<RigidBody>().build();

// collect collision pairs
ecs.system<RigidBody>().each([q1](flecs::entity rb1) {
    q1.each([](flecs::entity rb2) {
        if (rb1 == rb2) continue; // {rb1, rb2} and {rb2, rb1} is two different pairs (vertex-face collision)
        ecs.entity().add<CollisionPair>({rb1, rb2});
    });
});

ecs.system<BVH, Geometry>("update_bvh").with<RigidBody>().each([](BVH& bvh, const Geometry& geom) {
    if (!bvh.initialized) {
        bvh.bvh_.init(geom.verts.data(), geom.verts.size());
        bvh.bvh_.construct();
        bvh.initialized = true;
    } else {
        bvh.bvh_.update();
    }
});

ecs.system<AABB, Geometry>("update_aabb").with<RigidBody>().each([](AABB& aabb, const Geometry& geom) {
    for (size_t i = 0; i < geom.verts.size(); ++i) {
        auto p = geom.verts.getPosition(i);

        if (aabb.m_p[0][0] > p[0])
            aabb.m_p[0][0] = p[0];
        if (aabb.m_p[0][1] > p[1])
            aabb.m_p[0][1] = p[1];
        if (aabb.m_p[0][2] > p[2])
            aabb.m_p[0][2] = p[2];
        if (aabb.m_p[1][0] < p[0])
            aabb.m_p[1][0] = p[0];
        if (aabb.m_p[1][1] < p[1])
            aabb.m_p[1][1] = p[1];
        if (aabb.m_p[1][2] < p[2])
            aabb.m_p[1][2] = p[2];
    }

    float tolerance = 0.01f;

    // Extend AABB by tolerance
	co->m_aabb.m_p[0][0] -= m_tolerance;
	co->m_aabb.m_p[0][1] -= m_tolerance;
	co->m_aabb.m_p[0][2] -= m_tolerance;
	co->m_aabb.m_p[1][0] += m_tolerance;
	co->m_aabb.m_p[1][1] += m_tolerance;
	co->m_aabb.m_p[1][2] += m_tolerance;
});

struct CollisionPair {
    flecs::entity e1;
    flecs::entity e2;
};

// an rb can have more than one collision object??
ecs.system<CollisionPair>().each([q3](flecs::iter& it, CollisionPair& pair){
    auto rb1 = pair.e1;
    auto rb2 = pair.e2;

    auto aabb1 = rb1.get<AABB>();
    auto aabb2 = rb2.get<AABB>();

    if (AABB::intersection(aabb1, aabb2)) return; // boadphase

    auto bvh1 = rb1.get<BVH>();
    auto bvh2 = rb2.get<BVH>();

    auto co1 = rb1.get<CollisionObject>();
    auto co2 = rb2.get<CollisionObject>();

    auto m1 = rb1.get<Mass>();
    auto m2 = rb2.get<Mass>();

    if ((m1 == 0) && (m2 == 0)) return; // don't report collision between static bodies

    auto verts1 = rb1.get<Mesh>().get_world_vertices(); // surface vertices, in world coord

    auto com2 = rb2.get<Position>();
    auto R2 = rb2.get<Rotation>();

    auto predicate = [&](unsigned int node_index, unsigned int depth) {}; // midphase

    // narrowphase
    auto cb = [&](unsigned int node_index, unsigned int depth) {

        ecs.entity().add<Contact>({contact_info});
    };
    bvh1.traverse_depth_first(predicate, cb);
});

```

# 할 거

kdtree 시각화해보기. 파티클 랜덤한 위치에 추가하면서
어떻게 kdtree 를 컴포넌트로 이용하는지

일단 씬에 있는 파티클 대상으로, 글로벌 kdtree를 운용해봐야지

m_bvh.init(vertices, numVertices);
m_bvh.construct();
m_bvh.update(); > 경계구 재계산: 모든 노드의 경계구 업데이트
m_bvh.updateVertices(); > 이거는 vertices 데이터가 바뀔때: 새 데이터 포인터 지정

이게 무슨 갯수랑 관련이있는것같음. 파티클 10개만있을때는 depth 가 0밖에 안생김. > 아무리 넓게 퍼뜨려도 안생김
근데 파티클 100개 만들면 depth 가 4까지 생김

암튼 브로드페이즈는 이걸로 하는것같음. 브로드 해서 뭘 수집하냐면,
