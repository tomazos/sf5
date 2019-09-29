#include <algorithm>
#include <filesystem>
#include <glm/glm.hpp>
#include <optional>
#include <png++/png.hpp>
#include <random>

#include "dvc/file.h"
#include "dvc/opts.h"
#include "dvc/program.h"
#include "dvc/python.h"

uint32_t DVC_OPTION(height, -, dvc::required, "image height");
uint32_t DVC_OPTION(width, -, dvc::required, "image width");
uint32_t DVC_OPTION(antialias, -, 1, "antialias samples");
std::string DVC_OPTION(output, o, dvc::required, "output image");

using glm::dvec2;
using glm::dvec3;

const dvec3 eye = {0, 0, 0};

const dvec3 topleft = {-1, 1, 1};
const dvec3 topright = {1, 1, 1};
const dvec3 botleft = {-1, -1, 1};
const dvec3 botright = {1, -1, 1};

const dvec3 center = topleft + 0.5 * (botright - topleft);
const dvec3 dir_right = (botright + topright) / 2.0 - center;
const dvec3 dir_up = (topleft + topright) / 2.0 - center;

namespace glm {
template <length_t L, typename T, qualifier Q>
std::ostream& operator<<(std::ostream& o, vec<L, T, Q> v) {
  o << "[ ";
  for (length_t i = 0; i < L; i++) o << v[i] << " ";
  o << "]";

  return o;
}
}  // namespace glm

using Color = dvec3;
using Direction = dvec3;
using Point = dvec3;

struct Material {
  double specular;
  double diffuse;
  double ambient;
  double shininess;
  Color color;
};

struct Ray {
  Point origin;
  Direction dir;

  Point at(double t) const { return origin + t * dir; }
};

struct Light {
  Point source;
  Color specular;
  Color diffuse;
};

dvec3 ambient = {1, 1, 1};

std::vector<Light> lights;

struct Hit {
  Point point;
  Direction normal;
};

class Object {
 public:
  virtual std::optional<Hit> collide(Ray ray) = 0;
  virtual Material material(Point point) = 0;
};

struct ObjectHit {
  Hit hit;
  Object* object = nullptr;
  Material material() { return object->material(hit.point); }
};

struct Scene {
  Color ambient;
  std::vector<Light> lights;
  std::vector<Object*> objects;

  std::optional<ObjectHit> collide(Ray ray) const {
    ObjectHit nearest_hit;
    for (Object* object : objects) {
      if (std::optional<Hit> candidate_hit = object->collide(ray)) {
        if (nearest_hit.object == nullptr ||
            distance(ray.origin, candidate_hit->point) <
                distance(ray.origin, nearest_hit.hit.point)) {
          nearest_hit.object = object;
          nearest_hit.hit = *candidate_hit;
        }
      }
    }
    if (nearest_hit.object == nullptr)
      return std::nullopt;
    else
      return nearest_hit;
  }
};

class Sphere : public Object {
 public:
  Sphere(dvec3 center, double radius, Material material)
      : center(center), radius(radius), material_(material) {}

  std::optional<Hit> collide(Ray ray) override {
    std::optional<dvec3> point = intersect_point(ray);

    if (!point) return std::nullopt;

    Hit hit;
    hit.point = *point;
    hit.normal = normalize(hit.point - center);
    return hit;
  }

  Material material(Point point) override { return material_; }

 private:
  std::optional<dvec3> intersect_point(Ray ray) {
    const double a = dot(ray.dir, ray.dir);
    const double b = dot(2.0 * ray.dir, ray.origin - center);
    const double c =
        dot(ray.origin - center, ray.origin - center) - radius * radius;

    if (b * b < 4 * a * c) return std::nullopt;

    const double d = std::sqrt(b * b - 4 * a * c);

    const double t1 = (-b + d) / (2 * a);
    const double t2 = (-b - d) / (2 * a);

    if (t1 < 0) {
      if (t2 < 0) {
        return std::nullopt;
      } else {
        return ray.at(t2);
      }
    } else {
      if (t2 < 0) {
        return ray.at(t1);
      } else {
        const dvec3 p1 = ray.at(t1);
        const dvec3 p2 = ray.at(t2);
        if (distance(ray.origin, p1) < distance(ray.origin, p2))
          return p1;
        else
          return p2;
      }
    }
  }

  dvec3 center;
  double radius;
  Material material_;
};

dvec3 shade(const Scene& scene, Ray ray, ObjectHit hit) {
  Material material = hit.material();
  Color color = scene.ambient * material.ambient;
  for (const Light& light : scene.lights) {
    dvec3 N = hit.hit.normal;
    dvec3 L = normalize(light.source - hit.hit.point);
    dvec3 R = 2.0 * dot(L, N) * N - L;
    dvec3 V = normalize(ray.origin - hit.hit.point);
    double D = dot(L, N);
    if (D <= 0) continue;
    color += material.diffuse * D * light.diffuse;
    double S = dot(R, V);
    if (S <= 0) continue;
    color +=
        material.specular * std::pow(S, material.shininess) * light.specular;
  }
  return color;
}

dvec3 render_pos(const Scene& scene, dvec2 pos) {
  dvec3 viewpoint = center + pos.x * dir_right + pos.y * dir_up;
  dvec3 dir = normalize(viewpoint - eye);
  Ray ray = {eye, dir};

  std::optional<ObjectHit> hit = scene.collide(ray);

  if (!hit) return {0, 0, 0};

  return shade(scene, ray, *hit);
}

void render_image(const Scene& scene) {
  png::image<png::rgba_pixel> image(height, width);

  double dx = 1.0 / width / antialias;
  double dy = 1.0 / height / antialias;

  for (uint32_t row = 0; row < height; row++) {
    double y = 1.0 - 2.0 * row / height;
    for (uint32_t col = 0; col < width; col++) {
      double x = 2.0 * col / width - 1.0;
      dvec3 color(0, 0, 0);
      for (uint32_t arow = 0; arow < antialias; arow++)
        for (uint32_t acol = 0; acol < antialias; acol++) {
          dvec2 pos(x + (1 + 2 * acol) * dx, y - (1 + 2 * arow) * dy);

          //          DVC_LOG("SAMPLE: ", col, " ", row, " ", acol, " ", arow, "
          //          ", pos.x,
          //                  " ", pos.y);
          color += render_pos(scene, pos);
        }

      color /= antialias * antialias;

      image[row][col] = {uint8_t(255 * std::clamp(color.r, 0.0, 1.0)),
                         uint8_t(255 * std::clamp(color.g, 0.0, 1.0)),
                         uint8_t(255 * std::clamp(color.b, 0.0, 1.0)), 255};
    }
  }

  image.write(output);
}

PyObject* hello(PyObject* self, PyObject* args) {
  DVC_LOG("Hello, World!");
  return Py_None;
}

PyMethodDef hello_def = {"hello", hello, METH_VARARGS, "Say hello"};

int main(int argc, char** argv) {
  dvc::program program(argc, argv);

  if (dvc::args.empty()) DVC_FAIL("Must specify input file");

  std::filesystem::path input_file = dvc::args.at(0);

  if (!exists(input_file)) DVC_FAIL("No such file: ", input_file);

  std::string code = dvc::load_file(input_file);

  Py_Initialize();

  PyObject* compiled = Py_CompileString(
      code.c_str(), input_file.string().c_str(), Py_file_input);

  if (!compiled) {
    PyErr_Print();
    std::exit(EXIT_FAILURE);
  }

  PyObject* global_dict = PyDict_New();
  PyDict_SetItemString(global_dict, "__builtins__", PyEval_GetBuiltins());

  PyObject* hello_function = PyCFunction_New(&hello_def, Py_None);

  PyDict_SetItemString(global_dict, "hello", hello_function);
  PyObject* local_dict = PyDict_New();

  PyObject* eval_code = PyEval_EvalCode(compiled, global_dict, local_dict);

  if (!eval_code) {
    PyErr_Print();
    std::exit(EXIT_FAILURE);
  }

  Py_Finalize();

  Scene scene;
  scene.ambient = {1, 1, 1};

  std::random_device r;
  std::default_random_engine e1(r());
  std::uniform_int_distribution<int> uniform_dist1(-10, 10);
  std::uniform_int_distribution<int> uniform_dist2(1, 10);

  scene.lights.push_back(Light{{1, 100, -100}, {1, 1, 1}, {1, 1, 1}});

  for (int i = 0; i < 100; i++) {
    scene.objects.push_back(new Sphere(
        {uniform_dist1(e1), uniform_dist1(e1), uniform_dist1(e1) + 30},
        uniform_dist2(e1), {0.33, 0.33, 0.33, 100, {1, 0, 0}}));
  }

  render_image(scene);
}
