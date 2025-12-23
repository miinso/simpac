#include <iostream>
#include "app.h"
#include "modules/engine/core/body.h"

void print(flecs::entity e) { std::cout << e.path() << " [" << e.type().str() << "]\n"; }

void scene1(flecs::world &w) {
    geometry::shape circle_shape;
    circle_shape.type = geometry::CIRCLE;
    circle_shape.density = 1.0f;
    circle_shape.circle.p = {0, 0};
    circle_shape.circle.radius = 1.0f;

    geometry::shape capsule_shape;
    capsule_shape.type = geometry::CAPSULE;
    capsule_shape.density = 1.0f;
    capsule_shape.capsule.p1 = {-1, -1};
    capsule_shape.capsule.p2 = {1, 1};
    capsule_shape.capsule.radius = 1.0f;

    // two bodies with their own shapes
    auto b1 = w.entity("body1").set<body::body>({42});
    auto &body1 = b1.get_mut<body::body>();
    body1.mass = 10;
    body1.type = body::DYNAMIC;

    auto b1_s1 = w.entity("shape1").child_of(b1).set<geometry::shape>(circle_shape);
    auto b1_s2 = w.entity("shape2").child_of(b1).set<geometry::shape>(capsule_shape);

    auto b2 = w.entity("body2").set<body::body>({43});
    auto &body2 = b2.get_mut<body::body>();
    body2.mass = 4;
    body2.type = body::DYNAMIC;
    auto b2_s1 = w.entity("shape1").child_of(b2).set<geometry::shape>(circle_shape);

    // and a rogue shape entity (so evil)
    auto ohno = w.entity("shape_ohno").child_of(b1_s1).set<geometry::shape>({});
}

void test1(flecs::world &w) {
    scene1(w);

    auto t1 = w.system<geometry::shape, body::body>(
                       "for all bodies, iterate all their direct (not grand) children")
                      .term_at(1)
                      .parent()
                      .kind(0) // manually run
                      .run([](flecs::iter &it) {
                          while (it.next()) {
                              // outer loop (per body)
                              printf("outer loop (per body)\n");
                              auto shape_list = it.field<geometry::shape>(0);
                              auto body_list = it.field<const body::body>(1);

                              // inner loop (per shape)
                              for (auto i: it) {
                                  printf("\t inner loop (per shape)\n");
                                  print(it.entity(i));
                                  //   printf("%d %d\n", shape_list[i].type, body_list[0].type);

                                  //   auto &shape = shape_list[i];
                                  //   auto info = geometry::compute_mass(shape);
                              }
                          }
                      });
    t1.run();

    // `it.field<const body::body>(1);` gives you a readonly list. but without `const`,
    // fatal: flecs.h: 34763: assert: !ecs_field_is_readonly(iter_, index) ACCESS_VIOLATION
}

void test1_a(flecs::world &w) {
    scene1(w);
    // equiv. to test1
    auto t1 = w.system<geometry::shape>(
                       "for all bodies, iterate all their direct (not grand) children")
                      .with<body::body>()
                      .parent()
                      .kind(0) // manually run
                      .run([](flecs::iter &it) {
                          while (it.next()) {
                              // outer loop (per body)
                              printf("outer loop (per body)\n");
                              auto shape_list = it.field<geometry::shape>(0);
                              auto body_list = it.field<body::body>(1);

                              // inner loop (per shape)
                              for (auto i: it) {
                                  printf("\t inner loop (per shape)\n");
                                  print(it.entity(i));
                              }
                          }
                      });
    t1.run();
    // fatal: flecs.h: 34763: assert: !ecs_field_is_readonly(iter_, index) ACCESS_VIOLATION
}

void test1_b(flecs::world &w) {
    scene1(w);
    // try access modifiers (https://www.flecs.dev/flecs/md_docs_2Queries.html#access-modifiers)
    auto t1 = w.system<geometry::shape>(
                       "for all bodies, iterate all their direct (not grand) children")
                      .with<body::body>()
                      .inout() // huh
                      //   .read_write()
                      .parent()
                      .kind(0)
                      .run([](flecs::iter &it) {
                          while (it.next()) {
                              // outer loop (per body)
                              printf("outer loop (per body)\n");
                              auto shape_list = it.field<geometry::shape>(0);
                              auto body_list = it.field<body::body>(1);

                              // inner loop (per shape)
                              for (auto i: it) {
                                  printf("\t inner loop (per shape)\n");
                                  print(it.entity(i));
                              }
                          }
                      });
    t1.run();

    // no crash!

    // NOTE: `inout()` and `read_write()` are two completly different things. i was bamboozled for
    // an hour. to modify the body component i need to use `inout()`

    // outer loop (per body)
    //     inner loop (per shape)
    //     ::body1::shape1 [geometry.shape, (Identifier,Name), (ChildOf,body1)]
    //     inner loop (per shape)
    //     ::body1::shape2 [geometry.shape, (Identifier,Name), (ChildOf,body1)]
    // outer loop (per body)
    //     inner loop (per shape)
    //     ::body2::shape1 [geometry.shape, (Identifier,Name), (ChildOf,body2)]
    // outer loop (per body)
    //     inner loop (per shape)
    //     ::body1::shape1::shape_ohno [geometry.shape, (Identifier,Name), (ChildOf,body1.shape1)]

    // (doc) When a component is matched through traversal and its access modifier is not explicitly
    // set, it defaults to flecs::In.
}

void test1_c(flecs::world &w) {
    scene1(w);
    // issue: system still runs on `shape_ohno`. i want direct children only
    auto t1 = w.system<geometry::shape, body::body>(
                       "for all bodies, iterate all their direct (not grand) children")
                      .term_at(1)
                      .inout()
                      .parent()
                      .cascade()
                      .kind(0)
                      .run([](flecs::iter &it) {
                          while (it.next()) {
                              // outer loop (per body)
                              printf("outer loop (per body)\n");
                              auto shape_list = it.field<geometry::shape>(0);
                              auto body_list = it.field<body::body>(1);

                              auto &body = body_list[0];
                              body.mass = 1009;
                              // inner loop (per shape)
                              for (auto i: it) {
                                  printf("\t inner loop (per shape)\n");
                                  print(it.entity(i));
                              }
                          }
                      });
    t1.run();
}


void test4(flecs::world &w) {
    scene1(w);

    // `compute_mass` routine with internal object types
    auto t1 = w.system<geometry::shape, body::body>("compute mass")
                      .term_at(1)
                      .inout()
                      .up()
                      .kind(0)
                      .run([](flecs::iter &it) {
                          while (it.next()) {
                              // outer loop (per body)
                              auto shape_list = it.field<geometry::shape>(0);
                              auto body_list = it.field<body::body>(1);

                              // body_list[0] works. subscript other than `0` crashes
                              auto &body = body_list[0];
                              body.mass = 0.0f;
                              body.inv_mass = 0.0f;
                              body.I = 0.0f;
                              body.inv_I = 0.0f;
                              body.local_center = Eigen::Vector2f::Zero();

                              if (body.type == body::STATIC || body.type == body::KINEMATIC) {
                                  body.position = body.origin;
                                  //   it.fini()
                                  //   it.skip()
                              }
                              assert(body.type == body::DYNAMIC);

                              float total_mass = 0;
                              Eigen::Vector2f center_of_masses = Eigen::Vector2f::Zero();
                              float total_inertia = 0; // body quantity

                              // inner loop (per shape)
                              for (auto i: it) {
                                  print(it.entity(i));
                                  //   printf("%d %d\n", shape_list[i].type, body_list[0].type);

                                  auto &shape = shape_list[i];
                                  auto info = geometry::compute_mass(shape);

                                  total_mass += info.mass;
                                  center_of_masses += info.center * info.mass; // mass weighted
                                  total_inertia += info.I;
                              }


                              if (total_mass > 0.0f) {
                                  body.mass += total_mass;
                                  body.inv_mass = 1.0f / body.mass;
                                  body.local_center = center_of_masses / body.mass;
                              }

                              if (total_inertia > 0.0f) {
                                  // account the translational offset (parallel axis thing)
                                  body.I = total_inertia;
                                  assert(body.I > 0.0f);

                                  body.inv_I = 1.0f / body.I;
                              } else {
                                  body.I = 0.0f;
                                  body.inv_I = 0.0f;
                              }

                              printf("mass: %f, I: %f, com: %2f, %2f \n", body.mass, body.I,
                                     body.local_center.x(), body.local_center.y());
                          }
                      });
    t1.run();

    // IDEA: run this thing when 1) a shape component is set and has a parent entity that owns a
    // body component or 2) existing shape entity with a shape component get it's parent assigned

    // https://github.com/erincatto/solver2d/blob/5f5ad3dbcac4fa57dbd49b47bf9dc340b37f378d/src/body.c#L152
}

void test5(flecs::world &w) {
    // register entities
    scene1(w);

    // get entity by name
    auto b1 = w.lookup("body1");

    // check if the lookup succeded
    assert(b1.is_valid());

    // print all direct children
    b1.children([](flecs::entity e) { print(e); });
}

void test3(flecs::world &w) {

    // objective: print event message when a shape entity is added to a parent body entity
    // m_world.observer

    // objective: print msg when a body owns a shape component through some child entity
}


int main() {
    App app = App("asd", 1, 1);
    auto world = app.m_world;

    // test1(world);
    // test1_a(world);
    // test1_b(world);
    // test1_c(world);
    // test2(world);
    test4(world);

    return 0;
}
