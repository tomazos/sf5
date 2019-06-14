#version 460
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
  mat4 mvp;
  mat4 imvp;
} ubo;

out gl_PerVertex {
  vec4 gl_Position;
};

layout(location = 0) out vec2 fragPos;
layout(location = 2) out mat4 imvp;

vec2 positions[6] = vec2[](
  vec2(-1, -1),
  vec2(+1, -1),
  vec2(-1, +1),
  vec2(+1, +1),
  vec2(-1, +1),
  vec2(+1, -1)
);

void main() {
  gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
  fragPos = positions[gl_VertexIndex];
  imvp = ubo.imvp;
}
