#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 fragPos;
layout(location = 1) in vec3 flookat;
layout(location = 2) in mat4 imvp;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D face0;
layout(binding = 2) uniform sampler2D face1;
layout(binding = 3) uniform sampler2D face2;
layout(binding = 4) uniform sampler2D face3;
layout(binding = 5) uniform sampler2D face4;
layout(binding = 6) uniform sampler2D face5;

void convert_xyz_to_cube_uv(float x, float y, float z,  out int index,  out float u,out float  v)
{
  float absX = abs(x);
  float absY = abs(y);
  float absZ = abs(z);
  
  bool isXPositive = x > 0;
  bool isYPositive = y > 0;
  bool isZPositive = z > 0;
  
  float maxAxis, uc, vc;
  
  // POSITIVE X
  if (isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from +z to -z
    // v (0 to 1) goes from -y to +y
    maxAxis = absX;
    uc = -z;
    vc = y;
    index = 0;
  }
  // NEGATIVE X
  if (!isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from -z to +z
    // v (0 to 1) goes from -y to +y
    maxAxis = absX;
    uc = z;
    vc = y;
    index = 1;
  }
  // POSITIVE Y
  if (isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from +z to -z
    maxAxis = absY;
    uc = x;
    vc = -z;
    index = 2;
  }
  // NEGATIVE Y
  if (!isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -z to +z
    maxAxis = absY;
    uc = x;
    vc = z;
    index = 3;
  }
  // POSITIVE Z
  if (isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -y to +y
    maxAxis = absZ;
    uc = x;
    vc = y;
    index = 4;
  }
  // NEGATIVE Z
  if (!isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from +x to -x
    // v (0 to 1) goes from -y to +y
    maxAxis = absZ;
    uc = -x;
    vc = y;
    index = 5;
  }

  // Convert range from -1 to 1 to 0 to 1
  u = 0.5f * (uc / maxAxis + 1.0f);
  v = 0.5f * (vc / maxAxis + 1.0f);
}

void main() {
  //outColor = vec4(fragPos / 2 + 0.5,0,1);
  //outColor = texture(face0, fragPos.xy / 2 + 0.5);
  
  vec4 pos = imvp * vec4(fragPos,0,1);
  
  vec3 d = normalize(pos.xyz);
  
  // outColor = vec4(d / 2 +0.5, 1);

  int face_index;
  float u;
  float v;
  convert_xyz_to_cube_uv(d.x, d.y, d.z, face_index, u, v);  

  switch (face_index) {
  case 0: outColor = texture(face0, vec2(u,v)); break;
  case 1: outColor = texture(face1, vec2(u,v)); break;
  case 2: outColor = texture(face2, vec2(u,v)); break;
  case 3: outColor = texture(face3, vec2(u,v)); break;
  case 4: outColor = texture(face4, vec2(u,v)); break;
  case 5: outColor = texture(face5, vec2(u,v)); break;
  }
}
