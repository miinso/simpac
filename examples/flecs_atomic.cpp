#include <flecs.h>
#include <Eigen/Dense>

// void SpawnEnemy(flecs::iter& it, size_t, const Game& g) {
//     const Level* lvl = g.level.get<Level>();

//     it.world().entity().child_of<enemies>().is_a<prefabs::Enemy>()
//         .set<Direction>({0})
//         .set<Position>({
//             lvl->spawn_point.x, 1.2, lvl->spawn_point.y
//         });
// }

struct Rigidbody {
    Eigen::Vector3f x;
};

int main(int, char *[]) {
    
    return 0;
}