#include "dvc/program.h"

#include <glm/glm.hpp>
#include <png++/png.hpp>
#include <random>

#include "dvc/opts.h"

uint32_t DVC_OPTION(width, -, dvc::required, "image face width");
size_t DVC_OPTION(num_stars, -, dvc::required, "number of stars");
float DVC_OPTION(limit, -, dvc::required, "limit to display in rads");

float random_signed_uniform() {
  thread_local std::random_device random_device;
  thread_local std::mt19937 mt19937(random_device());
  thread_local std::uniform_real_distribution<float> distribution(-1, 1);
  return distribution(mt19937);
}

glm::vec3 random_sphere_point() {
  while (true) {
    glm::vec3 point = {random_signed_uniform(), random_signed_uniform(),
                       random_signed_uniform()};
    if (length(point) <= 1) return point;
  }
}

float angle(glm::vec3 a, glm::vec3 b) { return std::acos(dot(a, b)); }

struct Star {
  Star()
      : location(random_sphere_point()),
        mag(length(location)),
        direction(normalize(location)) {}

  float angle(glm::vec3 direction) const {
    return ::angle(this->direction, direction);
  }

  glm::vec3 location;
  float mag;
  glm::vec3 direction;
};

struct CubeMap {
  CubeMap() : images(6, {width, width}) {}

  std::vector<png::image<png::rgba_pixel>> images;

  template <typename F>
  void shade_fragments(F f) {
    for (size_t face = 0; face < 6; face++)
      for (size_t x = 0; x < width; x++)
        for (size_t y = 0; y < width; y++) {
          glm::vec2 t =
              ((glm::vec2(x, y) + 0.5f) * (1.0f / width)) * 2.0f - 1.0f;
          glm::vec3 direction;
          switch (face) {
            case 0:
              direction = glm::vec3(t.x, t.y, 1);
              break;
            case 1:
              direction = glm::vec3(t.x, t.y, -1);
              break;
            case 2:
              direction = glm::vec3(1, t.x, t.y);
              break;
            case 3:
              direction = glm::vec3(-1, t.x, t.y);
              break;
            case 4:
              direction = glm::vec3(t.y, 1, t.x);
              break;
            case 5:
              direction = glm::vec3(t.y, -1, t.x);
              break;
          }
          direction = normalize(direction);
          glm::vec3 color;
          color = f(direction);

          images[face][x][y] = {uint8_t(255 * color.r), uint8_t(255 * color.g),
                                uint8_t(255 * color.b), 255};
        }
  }

  void write(const std::string basename) {
    for (size_t i = 0; i < images.size(); i++)
      images[i].write(dvc::concat(basename, i, ".png"));
  }
};

int main(int argc, char** argv) {
  dvc::program program(argc, argv);

  CubeMap cube_map;

  const std::vector<Star> stars(num_stars);

  size_t num_groups = std::sqrt(num_stars);

  std::vector<std::vector<uint32_t>> groups(num_groups);
  std::vector<float> group_spread(num_groups);

  for (size_t star = 0; star < num_stars; star++) {
    size_t closest_group = 0;
    float best_angle = std::numeric_limits<float>::max();
    for (size_t group = 0; group < num_groups; group++) {
      float candidate_angle = stars[star].angle(stars[group].direction);
      if (candidate_angle < best_angle) {
        best_angle = candidate_angle;
        closest_group = group;
      }
    }
    groups[closest_group].push_back(star);
    if (best_angle > group_spread[closest_group])
      group_spread[closest_group] = best_angle;
  }

  cube_map.shade_fragments([&](glm::vec3 direction) -> glm::vec3 {
    for (size_t group = 0; group < num_groups; group++) {
      float group_center_angle = stars[group].angle(direction);
      float group_min_angle = group_center_angle - group_spread[group];
      if (group_min_angle < limit) {
        for (size_t star : groups[group]) {
          float star_angle = stars[star].angle(direction);
          if (star_angle < limit) {
            return glm::vec3(1 - star_angle / limit);
          }
        }
      }
    }
    return {0, 0, 0};
  });

  cube_map.write("/home/zos/face");
}
