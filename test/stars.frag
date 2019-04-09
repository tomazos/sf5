#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragPos;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
  outColor = texture(texSampler, fragPos / 2 + 0.5);
  //outColor = vec4(fragPos / 2 + 0.5,0,1);
}
