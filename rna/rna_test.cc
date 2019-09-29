#include "rna/arena.h"
#include "rna/geometry.h"

#include "dvc/log.h"

int main() {
  using namespace rna;

  DVC_ASSERT_EQ(length(Vec3(1, 1, 1)), std::sqrt(3));
  DVC_ASSERT_EQ(distance(Vec3(1, 1, 1), Vec3(2, 2, 2)), std::sqrt(3));
  DVC_ASSERT_EQ(dot(Vec3(1, 2, 3), Vec3(10, 20, 30)), 1 * 10 + 2 * 20 + 3 * 30);
  DVC_ASSERT_EQ(cross(Vec3(1, 2, 3), Vec3(4, 5, 6)),
                Vec3(2 * 6 - 3 * 5, 3 * 4 - 1 * 6, 1 * 5 - 2 * 4));
  Vec3 v(1, 2, 3);
  DVC_ASSERT_EQ(normalize(v) * length(v), v);
  //  DVC_ASSERT_EQ(faceforward(Vec3(1,0,0), Vec3(0,1,0), Vec3(0,2,1)),

  //  DVC_ASSERT_EQ(distance(a, b), 1);

  //  rna::Arena arena;
  //
  //  auto object = std::make_unique<rna::Object>("foo");
  //  object->position = rna::Vec3(1, 2, 3);
  //  object->velocity = rna::Vec3(1, 0, 0);
  //  arena.add_object(std::move(object));
}
