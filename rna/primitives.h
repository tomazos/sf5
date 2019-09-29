#pragma once

#include <glm/glm.hpp>

namespace rna {

using Vec3 = glm::dvec3;

using Scalar = double;

}  // namespace rna

namespace glm {

template <length_t L, typename T, qualifier Q>
std::ostream& operator<<(std::ostream& o, vec<L, T, Q> v) {
  o << "[ ";
  for (length_t i = 0; i < L; i++) o << v[i] << " ";
  o << "]";

  return o;
}

}  // namespace glm
