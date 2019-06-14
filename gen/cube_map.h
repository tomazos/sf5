#pragma once

struct CubeMap {
  CubeMap(uint32_t width, uint32_t oversample) : images(6, {width, width}) {}

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
