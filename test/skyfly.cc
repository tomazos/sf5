#define GLM_ENABLE_EXPERIMENTAL

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <png++/png.hpp>
#include <random>
#include <set>

#include <iostream>
#include "dvc/file.h"
#include "dvc/log.h"
#include "dvc/opts.h"
#include "dvc/terminate.h"
#include "spk/game.h"
#include "spk/helpers.h"
#include "spk/loader.h"
#include "spk/memory.h"
#include "spk/spock.h"

namespace {

bool DVC_OPTION(trace_allocations, -, false, "trace vulkan allocations");
uint64_t DVC_OPTION(num_points, -, 100, "num of points");

struct Pipeline {
  //  spk::shader_module vertex_shader, fragment_shader;
  spk::pipeline_layout pipeline_layout;
  spk::render_pass render_pass;
  spk::pipeline pipeline;
};

struct UniformBufferObject {
  glm::mat4 model = glm::mat4(1.0);
  glm::mat4 view = glm::mat4(1.0);
  glm::mat4 proj = glm::mat4(1.0);
};

spk::descriptor_set_layout create_descriptor_set_layout(spk::device& device) {
  spk::descriptor_set_layout_binding binding[2];
  binding[0].set_binding(0);
  binding[0].set_descriptor_type(spk::descriptor_type::uniform_buffer);
  binding[0].set_immutable_samplers({nullptr, 1});
  binding[0].set_stage_flags(spk::shader_stage_flags::vertex);

  binding[1].set_binding(1);
  binding[1].set_descriptor_type(spk::descriptor_type::combined_image_sampler);
  binding[1].set_immutable_samplers({nullptr, 1});
  binding[1].set_stage_flags(spk::shader_stage_flags::fragment);

  spk::descriptor_set_layout_create_info create_info;
  create_info.set_bindings({binding, 2});
  return device.create_descriptor_set_layout(create_info);
}

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
};

struct PointMass {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec3 velocity;
  void update(float alpha) {
    pos += alpha * velocity;

    auto adjust = [](float& pos, float& vel) {
      if (pos > 1 && vel > 0) vel = -vel;
      if (pos < -1 && vel < 0) vel = -vel;
    };
    adjust(pos.x, velocity.x);
    adjust(pos.y, velocity.y);
    adjust(pos.z, velocity.z);
  }
};

spk::vertex_input_binding_description get_vertex_input_binding_description() {
  spk::vertex_input_binding_description vertex_input_binding_description;
  vertex_input_binding_description.set_binding(0);
  vertex_input_binding_description.set_input_rate(
      spk::vertex_input_rate::vertex);
  vertex_input_binding_description.set_stride(sizeof(Vertex));
  return vertex_input_binding_description;
}

std::vector<spk::vertex_input_attribute_description>
get_vertex_input_attribute_descriptions() {
  std::vector<spk::vertex_input_attribute_description> result(2);
  result[0].set_binding(0);
  result[0].set_location(0);
  result[0].set_format(spk::format::r32g32b32_sfloat);
  result[0].set_offset(offsetof(Vertex, pos));
  result[1].set_binding(0);
  result[1].set_location(1);
  result[1].set_offset(offsetof(Vertex, color));
  result[1].set_format(spk::format::r32g32b32_sfloat);
  return result;
}

// spk::device_memory create_memory(spk::physical_device& physical_device,
//                                 spk::device& device, spk::buffer& buffer) {
//  const spk::memory_requirements memory_requirements =
//      buffer.memory_requirements();
//  spk::memory_allocate_info memory_allocate_info;
//  memory_allocate_info.set_allocation_size(memory_requirements.size());
//  memory_allocate_info.set_memory_type_index(spkx::find_compatible_memory_type(
//      physical_device, memory_requirements.memory_type_bits(),
//      spk::memory_property_flags::host_visible |
//          spk::memory_property_flags::host_coherent));
//  return device.allocate_memory(memory_allocate_info);
//}

struct Object {
  glm::vec3 pos;
  glm::vec3 fac;
  glm::vec3 vel;
  glm::vec3 up;
  float dir = 0;

  void normalize() {
    fac = glm::normalize(fac);
    up = glm::normalize(up - dot(up, fac) * fac);
  }
};

struct World {
  Object player;

  World(size_t num_points) : num_points(num_points), points(num_points) {
    rng.seed(std::random_device()());

    for (size_t i = 0; i < num_points; ++i) {
      points[i].pos = glm::vec3{normal(), normal(), normal()};
      points[i].color = glm::vec3{normal(), normal(), normal()};
      points[i].velocity = glm::vec3{normal(), normal(), normal()};
      points[i].update(0);
    }

    player.pos = {-20, -20, -20};
    player.fac = {20, 20, 20};
    player.vel = {0, 0, 0};
    player.up = {0, 1, 0};
    player.normalize();
  }

  void update(float alpha) {
    for (auto& point : points) point.update(alpha);

    player.vel += player.dir * player.fac * 0.001f;
    player.pos += player.vel;
  }

  float normal() { return normal_(rng); }

  void move_head(glm::vec2 headrel) { /*static constexpr float k = 0.01; */
    player.fac = glm::rotate(player.fac, headrel.x * -0.001f, player.up);
    player.normalize();
    player.fac = glm::rotate(player.fac, headrel.y * 0.001f,
                             glm::normalize(glm::cross(player.fac, player.up)));
    player.normalize();
  }

  void go_forward() { player.dir += 1; }
  void go_backward() { player.dir -= 0.2; }
  void go_left() { DVC_LOG("go_left"); }
  void go_right() { DVC_LOG("go_right"); }
  void stop_forward() { player.dir -= 1; }
  void stop_backward() { player.dir += 0.2; }
  void stop_left() { DVC_LOG("stop_left"); }
  void stop_right() { DVC_LOG("stop_right"); }

  std::normal_distribution<float> normal_;
  std::mt19937 rng;
  size_t num_points;
  std::vector<PointMass> points;
};

struct VertexBuffer {
  spk::buffer buffer;
  spk::device_memory device_memory;
  uint64_t size;
  void* data;

  void map() { device_memory.map_memory(0, size, data); }
  void unmap() { device_memory.unmap_memory(); }
  void update(const World& world) {
    Vertex* v = (Vertex*)data;
    for (size_t i = 0; i < world.num_points; ++i) {
      v[i].color = world.points[i].color;
      v[i].pos = world.points[i].pos;
    }
  }
};

VertexBuffer create_vertex_buffer(spk::physical_device& physical_device,
                                  spk::device& device) {
  spk::buffer buffer =
      spkx::create_buffer(device, sizeof(Vertex) * ::num_points,
                          spk::buffer_usage_flags::vertex_buffer);
  const spk::memory_requirements memory_requirements =
      buffer.memory_requirements();
  spk::device_memory device_memory = spkx::create_memory(
      device, memory_requirements.size(),
      spkx::find_compatible_memory_type(
          physical_device, memory_requirements.memory_type_bits(),
          spk::memory_property_flags::host_visible |
              spk::memory_property_flags::host_coherent));

  buffer.bind_memory(device_memory, 0);

  return {std::move(buffer), std::move(device_memory),
          memory_requirements.size()};
}

struct UniformBuffer {
  spk::buffer buffer;
  spk::device_memory device_memory;
  uint64_t size;
  void* data;

  void map() { device_memory.map_memory(0, size, data); }
  void unmap() { device_memory.unmap_memory(); }
  void update(const UniformBufferObject& ubo) { std::memcpy(data, &ubo, size); }
};

UniformBuffer create_uniform_buffer(spk::physical_device& physical_device,
                                    spk::device& device) {
  spk::buffer buffer =
      spkx::create_buffer(device, sizeof(UniformBufferObject),
                          spk::buffer_usage_flags::uniform_buffer);
  const spk::memory_requirements memory_requirements =
      buffer.memory_requirements();
  spkx::memory_type_index memory_type_index = spkx::find_compatible_memory_type(
      physical_device, memory_requirements.memory_type_bits(),
      spk::memory_property_flags::host_visible |
          spk::memory_property_flags::host_coherent);
  spk::device_memory device_memory = spkx::create_memory(
      device, memory_requirements.size(), memory_type_index);
  buffer.bind_memory(device_memory, 0);

  return {std::move(buffer), std::move(device_memory),
          memory_requirements.size()};
}

spk::pipeline_layout create_pipeline_layout(
    spk::device& device, spk::descriptor_set_layout& descriptor_set_layout) {
  spk::pipeline_layout_create_info create_info;
  spk::descriptor_set_layout_ref descriptor_set_layout_ref =
      descriptor_set_layout;
  create_info.set_set_layouts({&descriptor_set_layout_ref, 1});
  return device.create_pipeline_layout(create_info);
}

spk::pipeline create_point_pipeline(spk::device& device,
                                    spkx::presenter& presenter,
                                    spk::pipeline_layout& pipeline_layout) {
  spkx::pipeline_config config;
  config.vertex_shader = "test/skyfly.vert.spv";
  config.fragment_shader = "test/skyfly.frag.spv";
  config.vertex_binding_descriptions.push_back(
      get_vertex_input_binding_description());
  config.vertex_attribute_descriptions =
      get_vertex_input_attribute_descriptions();
  config.topology = spk::primitive_topology::point_list;
  config.layout = pipeline_layout;

  return spkx::create_pipeline(device, presenter, config);
}

[[maybe_unused]] spk::pipeline create_stars_pipeline(
    spk::device& device, spkx::presenter& presenter,
    spk::pipeline_layout& pipeline_layout) {
  spkx::pipeline_config config;
  config.vertex_shader = "test/stars.vert.spv";
  config.fragment_shader = "test/stars.frag.spv";
  config.topology = spk::primitive_topology::triangle_list;
  config.layout = pipeline_layout;

  return spkx::create_pipeline(device, presenter, config);
}

std::vector<VertexBuffer> create_vertex_buffers(
    size_t num_renderings, spk::physical_device& physical_device,
    spk::device& device) {
  std::vector<VertexBuffer> buffers;
  for (size_t i = 0; i < num_renderings; ++i)
    buffers.emplace_back(create_vertex_buffer(physical_device, device));
  for (VertexBuffer& buffer : buffers) buffer.map();
  return buffers;
}

std::vector<UniformBuffer> create_uniform_buffers(
    size_t num_renderings, spk::physical_device& physical_device,
    spk::device& device) {
  std::vector<UniformBuffer> buffers;
  for (size_t i = 0; i < num_renderings; ++i)
    buffers.emplace_back(create_uniform_buffer(physical_device, device));
  for (UniformBuffer& buffer : buffers) buffer.map();
  return buffers;
}

spk::descriptor_pool create_descriptor_pool(spk::device& device,
                                            uint32_t pool_size) {
  spk::descriptor_pool_create_info create_info;
  create_info.set_max_sets(pool_size);
  spk::descriptor_pool_size size[2];
  size[0].set_descriptor_count(pool_size);
  size[0].set_type(spk::descriptor_type::uniform_buffer);
  size[1].set_descriptor_count(pool_size);
  size[1].set_type(spk::descriptor_type::combined_image_sampler);
  create_info.set_pool_sizes({size, 2});
  return device.create_descriptor_pool(create_info);
}

std::vector<spk::descriptor_set> create_descriptor_sets(
    spk::device& device, spk::descriptor_pool& descriptor_pool,
    spk::descriptor_set_layout& layout, uint32_t num_descriptors) {
  spk::descriptor_set_allocate_info allocate_info;
  allocate_info.set_descriptor_pool(descriptor_pool);
  std::vector<spk::descriptor_set_layout_ref> layouts(num_descriptors, layout);
  allocate_info.set_set_layouts({layouts.data(), num_descriptors});
  return device.allocate_descriptor_sets(allocate_info);
}

struct ImageBuffer {
  uint64_t size;
  spk::image image;
  spk::device_memory image_memory;
  spk::image_view image_view;
  spk::sampler sampler;
};

spk::image create_image(spk::device& device, uint32_t width, uint32_t height,
                        spk::format format) {
  spk::image_create_info create_info;
  create_info.set_image_type(spk::image_type::n2d);
  spk::extent_3d extent;
  extent.set_width(width);
  extent.set_height(height);
  extent.set_depth(1);
  create_info.set_extent(extent);
  create_info.set_mip_levels(1);
  create_info.set_array_layers(1);
  create_info.set_format(format);
  create_info.set_tiling(spk::image_tiling::optimal);
  create_info.set_initial_layout(spk::image_layout::undefined);
  create_info.set_usage(spk::image_usage_flags::transfer_dst |
                        spk::image_usage_flags::sampled);
  create_info.set_sharing_mode(spk::sharing_mode::exclusive);
  create_info.set_samples(spk::sample_count_flags::n1);
  return device.create_image(create_info);
}

void immediate_copy_image(spk::device& device, spk::queue& transfer_queue,
                          spk::queue& graphics_queue,
                          uint32_t transfer_queue_family_index,
                          uint32_t graphics_queue_family_index,
                          spk::buffer& host_buffer, spk::image& image,
                          uint32_t width, uint32_t height) {
  spk::command_pool_create_info create_info;
  create_info.set_flags(spk::command_pool_create_flags::transient);
  create_info.set_queue_family_index(transfer_queue_family_index);
  spk::command_pool command_pool = device.create_command_pool(create_info);

  spk::command_buffer_allocate_info command_buffer_allocate_info;
  command_buffer_allocate_info.set_command_pool(command_pool);
  command_buffer_allocate_info.set_level(spk::command_buffer_level::primary);
  command_buffer_allocate_info.set_command_buffer_count(1);
  spk::command_buffer command_buffer = std::move(
      device.allocate_command_buffers(command_buffer_allocate_info).at(0));

  spk::command_buffer_begin_info begin_info;
  begin_info.set_flags(spk::command_buffer_usage_flags::one_time_submit);
  command_buffer.begin(begin_info);

  {
    spk::image_memory_barrier barrier;
    barrier.set_old_layout(spk::image_layout::undefined);
    barrier.set_new_layout(spk::image_layout::transfer_dst_optimal);
    barrier.set_src_queue_family_index(spk::queue_family_ignored);
    barrier.set_dst_queue_family_index(spk::queue_family_ignored);
    barrier.set_image(image);
    barrier.set_src_access_mask({});
    barrier.set_dst_access_mask(spk::access_flags::transfer_write);

    spk::image_subresource_range range;
    range.set_aspect_mask(spk::image_aspect_flags::color);
    range.set_level_count(1);
    range.set_layer_count(1);
    barrier.set_subresource_range(range);

    command_buffer.pipeline_barrier(spk::pipeline_stage_flags::top_of_pipe,
                                    spk::pipeline_stage_flags::transfer, {}, {},
                                    {}, {&barrier, 1});
  }

  spk::buffer_image_copy region;

  spk::image_subresource_layers layers;
  layers.set_aspect_mask(spk::image_aspect_flags::color);
  layers.set_layer_count(1);
  region.set_image_subresource(layers);

  spk::extent_3d extent;
  extent.set_width(width);
  extent.set_height(height);
  extent.set_depth(1);
  region.set_image_extent(extent);

  command_buffer.copy_buffer_to_image(host_buffer, image,
                                      spk::image_layout::transfer_dst_optimal,
                                      {&region, 1});

  {
    spk::image_memory_barrier barrier;
    barrier.set_old_layout(spk::image_layout::transfer_dst_optimal);
    barrier.set_new_layout(spk::image_layout::shader_read_only_optimal);
    barrier.set_src_queue_family_index(transfer_queue_family_index);
    barrier.set_dst_queue_family_index(graphics_queue_family_index);
    barrier.set_image(image);
    barrier.set_src_access_mask(spk::access_flags::transfer_write);
    barrier.set_dst_access_mask(spk::access_flags::shader_read);

    spk::image_subresource_range range;
    range.set_aspect_mask(spk::image_aspect_flags::color);
    range.set_level_count(1);
    range.set_layer_count(1);
    barrier.set_subresource_range(range);

    command_buffer.pipeline_barrier(spk::pipeline_stage_flags::transfer,
                                    spk::pipeline_stage_flags::fragment_shader,
                                    {}, {}, {}, {&barrier, 1});
  }

  command_buffer.end();
  spk::submit_info submit_info;
  spk::command_buffer_ref ref = command_buffer;
  submit_info.set_command_buffers({&ref, 1});
  transfer_queue.submit({&submit_info, 1}, VK_NULL_HANDLE);
  transfer_queue.wait_idle();
  command_pool.free_command_buffers({&ref, 1});
}

spk::image_view create_image_view(spk::device& device, spk::image& image) {
  spk::image_view_create_info info;
  info.set_image(image);
  info.set_view_type(spk::image_view_type::n2d);
  info.set_format(spk::format::r8g8b8a8_unorm);

  spk::image_subresource_range range;
  range.set_aspect_mask(spk::image_aspect_flags::color);
  range.set_level_count(1);
  range.set_layer_count(1);
  info.set_subresource_range(range);

  return device.create_image_view(info);
}

spk::sampler create_texture_sampler(spk::device& device) {
  spk::sampler_create_info info;
  info.set_mag_filter(spk::filter::linear);
  info.set_min_filter(spk::filter::linear);
  info.set_address_mode_u(spk::sampler_address_mode::repeat);
  info.set_address_mode_v(spk::sampler_address_mode::repeat);
  info.set_address_mode_w(spk::sampler_address_mode::repeat);
  info.set_anisotropy_enable(true);
  info.set_max_anisotropy(16);
  info.set_border_color(spk::border_color::int_opaque_black);
  info.set_unnormalized_coordinates(false);
  info.set_compare_enable(true);
  info.set_compare_op(spk::compare_op::always);
  info.set_mipmap_mode(spk::sampler_mipmap_mode::linear);
  return device.create_sampler(info);
}

ImageBuffer create_image_buffer(spk::physical_device& physical_device,
                                spk::device& device, spk::queue& transfer_queue,
                                spk::queue& graphics_queue,
                                uint32_t transfer_queue_family_index,
                                uint32_t graphics_queue_family_index) {
  png::image<png::rgba_pixel, png::solid_pixel_buffer<png::rgba_pixel>> image(
      "test/starfield.png");
  DVC_DUMP(image.get_width());
  DVC_DUMP(image.get_height());

  const std::vector<png::byte>& bytes = image.get_pixbuf().get_bytes();

  DVC_ASSERT_EQ(image.get_width() * image.get_height() * 4, bytes.size());

  spk::buffer buffer = spkx::create_buffer(
      device, bytes.size(), spk::buffer_usage_flags::transfer_src);

  const spk::memory_requirements memory_requirements =
      buffer.memory_requirements();
  spk::device_memory device_memory = spkx::create_memory(
      device, memory_requirements.size(),
      spkx::find_compatible_memory_type(
          physical_device, memory_requirements.memory_type_bits(),
          spk::memory_property_flags::host_visible |
              spk::memory_property_flags::host_coherent));
  buffer.bind_memory(device_memory, 0);

  void* buf;
  device_memory.map_memory(0, bytes.size(), buf);
  std::memcpy(buf, bytes.data(), bytes.size());
  device_memory.unmap_memory();

  spk::image device_image =
      create_image(device, image.get_width(), image.get_height(),
                   spk::format::r8g8b8a8_unorm);

  const spk::memory_requirements image_memory_requirements =
      device_image.memory_requirements();

  spk::device_memory image_device_memory = spkx::create_memory(
      device, image_memory_requirements.size(),
      spkx::find_compatible_memory_type(
          physical_device, image_memory_requirements.memory_type_bits(),
          spk::memory_property_flags::device_local));
  device_image.bind_memory(image_device_memory, 0);

  immediate_copy_image(device, transfer_queue, graphics_queue,
                       transfer_queue_family_index, graphics_queue_family_index,
                       buffer, device_image, image.get_width(),
                       image.get_height());

  device.free_memory(device_memory);

  spk::image_view image_view = create_image_view(device, device_image);

  spk::sampler sampler = create_texture_sampler(device);

  return {memory_requirements.size(), std::move(device_image),
          std::move(image_device_memory), std::move(image_view),
          std::move(sampler)};
}

struct SkyFly : spkx::game {
  World world;
  spk::descriptor_set_layout descriptor_set_layout;
  spk::pipeline_layout pipeline_layout;
  spk::pipeline point_pipeline, stars_pipeline;
  std::vector<VertexBuffer> vertex_buffers;
  std::vector<UniformBuffer> uniform_buffers;
  ImageBuffer image_buffer;
  spk::descriptor_pool descriptor_pool;
  std::vector<spk::descriptor_set> descriptor_sets;

  SkyFly(int argc, char** argv)
      : spkx::game(argc, argv),
        world(::num_points),
        descriptor_set_layout(create_descriptor_set_layout(device())),
        pipeline_layout(
            create_pipeline_layout(device(), descriptor_set_layout)),
        point_pipeline(
            create_point_pipeline(device(), presenter(), pipeline_layout)),
        stars_pipeline(
            create_stars_pipeline(device(), presenter(), pipeline_layout)),
        vertex_buffers(create_vertex_buffers(num_renderings(),
                                             physical_device(), device())),
        uniform_buffers(create_uniform_buffers(num_renderings(),
                                               physical_device(), device())),
        image_buffer(create_image_buffer(
            physical_device(), device(), transfer_queue(), graphics_queue(),
            transfer_queue_family(), graphics_queue_family())),
        descriptor_pool(create_descriptor_pool(device(), num_renderings())),
        descriptor_sets(create_descriptor_sets(device(), descriptor_pool,
                                               descriptor_set_layout,
                                               num_renderings())) {
    DVC_ASSERT_EQ(uniform_buffers.size(), descriptor_sets.size());
    DVC_ASSERT_EQ(uniform_buffers.size(), num_renderings());
    for (size_t i = 0; i < num_renderings(); i++) {
      spk::descriptor_buffer_info buffer_info;
      buffer_info.set_buffer(uniform_buffers[i].buffer);
      buffer_info.set_offset(0);
      buffer_info.set_range(sizeof(UniformBufferObject));

      spk::descriptor_image_info image_info;
      image_info.set_image_layout(spk::image_layout::shader_read_only_optimal);
      image_info.set_image_view(image_buffer.image_view);
      image_info.set_sampler(image_buffer.sampler);

      spk::write_descriptor_set write[2];
      write[0].set_buffer_info({&buffer_info, 1});
      write[0].set_descriptor_type(spk::descriptor_type::uniform_buffer);
      write[0].set_dst_array_element(0);
      write[0].set_dst_binding(0);
      write[0].set_dst_set(descriptor_sets[i]);
      write[0].set_image_info({nullptr, 1});
      write[0].set_texel_buffer_view({nullptr, 1});

      write[1].set_dst_set(descriptor_sets[i]);
      write[1].set_dst_binding(1);
      write[1].set_dst_array_element(0);
      write[1].set_descriptor_type(
          spk::descriptor_type::combined_image_sampler);

      write[1].set_image_info({&image_info, 1});

      device().update_descriptor_sets({write, 2}, {nullptr, 0});
    }
  }

  void tick() override {}

  void prepare_rendering(
      spk::command_buffer& command_buffer, size_t rendering_index,
      spk::render_pass_begin_info& render_pass_begin_info) override {
    // std::terminate();
    VertexBuffer& current_buffer = vertex_buffers[rendering_index];
    UniformBuffer& uniform_buffer = uniform_buffers[rendering_index];

    world.update(0.01);
    current_buffer.update(world);

    UniformBufferObject ubo;
    ubo.view = glm::lookAt(
        world.player.pos, world.player.pos + world.player.fac, world.player.up);
    ubo.proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    uniform_buffer.update(ubo);
    spk::clear_color_value clear_color_value;
    clear_color_value.set_float_32({0, 0, 0, 1});

    spk::clear_value clear_color;
    clear_color.set_color(clear_color_value);
    render_pass_begin_info.set_clear_values({&clear_color, 1});

    spk::descriptor_set_ref descriptor_set_ref =
        descriptor_sets.at(rendering_index);

    command_buffer.begin_render_pass(render_pass_begin_info,
                                     spk::subpass_contents::inline_);

    command_buffer.bind_pipeline(spk::pipeline_bind_point::graphics,
                                 stars_pipeline);
    command_buffer.bind_descriptor_sets(spk::pipeline_bind_point::graphics,
                                        pipeline_layout, 0,
                                        {&descriptor_set_ref, 1}, {nullptr, 0});

    command_buffer.draw(6, 1, 0, 0);

    command_buffer.bind_pipeline(spk::pipeline_bind_point::graphics,
                                 point_pipeline);
    spk::buffer_ref buffer_ref = current_buffer.buffer;
    uint64_t offset = 0;
    command_buffer.bind_vertex_buffers(0, 1, &buffer_ref, &offset);
    command_buffer.bind_descriptor_sets(spk::pipeline_bind_point::graphics,
                                        pipeline_layout, 0,
                                        {&descriptor_set_ref, 1}, {nullptr, 0});
    command_buffer.draw(::num_points, 1, 0, 0);

    command_buffer.end_render_pass();
  }

  void mouse_motion(const mouse_motion_event& event) override {
    world.move_head(glm::vec2{event.xrel, event.yrel});
  };

  virtual void key_down(const keyboard_event& event) {
    if (event.keysym.sym == SDLK_q) shutdown();

    if (event.repeat) return;

    switch (event.keysym.sym) {
      case SDLK_w:
        world.go_forward();
        break;
      case SDLK_s:
        world.go_backward();
        break;
      case SDLK_a:
        world.go_left();
        break;
      case SDLK_d:
        world.go_right();
        break;
    }
  }

  virtual void key_up(const keyboard_event& event) {
    switch (event.keysym.sym) {
      case SDLK_w:
        world.stop_forward();
        break;
      case SDLK_s:
        world.stop_backward();
        break;
      case SDLK_a:
        world.stop_left();
        break;
      case SDLK_d:
        world.stop_right();
        break;
    }
  }

  ~SkyFly() {
    device().free_memory(image_buffer.image_memory);
    for (VertexBuffer& buffer : vertex_buffers) {
      buffer.unmap();
      device().free_memory(buffer.device_memory);
    }
    for (UniformBuffer& buffer : uniform_buffers) {
      buffer.unmap();
      device().free_memory(buffer.device_memory);
    }
  }
};

}  // namespace

int main(int argc, char** argv) {
  SkyFly skyfly(argc, argv);
  skyfly.run();
}
