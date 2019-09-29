#pragma once

#include "rna/primitives.h"

namespace rna {

class Object {
 public:
  Object(const std::string& name) : name(name) {}

  std::string name;
  Vec3 position;
  Vec3 velocity;
  Scalar radius;
};

}  // namespace rna
