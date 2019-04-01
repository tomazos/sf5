#pragma once

#include "rna/primitives.h"

namespace rna {

class Object {
  Vec3 position;
  Scalar radius;
  Vec3 velocity;
  Vec3 rotation_axis;
  Scalar rotation_rate;
};

}  // namespace rna
