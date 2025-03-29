#version 330 core
layout(location = 0) out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D tex0;
uniform vec3 ColorMask;
const vec2 kernel_i7x7[49] = vec2[49](
  vec2(-3, +3), vec2(-2, +3), vec2(-1, +3), vec2(+0, +3), vec2(+1, +3), vec2(+2, +3), vec2(+3, +3),
  vec2(-3, +2), vec2(-2, +2), vec2(-1, +2), vec2(+0, +2), vec2(+1, +2), vec2(+2, +2), vec2(+3, +2),
  vec2(-3, +1), vec2(-2, +1), vec2(-1, +1), vec2(+0, +1), vec2(+1, +1), vec2(+2, +1), vec2(+3, +1),
  vec2(-3, +0), vec2(-2, +0), vec2(-1, +0), vec2(+0, +0), vec2(+1, +0), vec2(+2, +0), vec2(+3, +0),
  vec2(-3, -1), vec2(-2, -1), vec2(-1, -1), vec2(+0, -1), vec2(+1, -1), vec2(+2, -1), vec2(+3, -1),
  vec2(-3, -2), vec2(-2, -2), vec2(-1, -2), vec2(+0, -2), vec2(+1, -2), vec2(+2, -2), vec2(+3, -2),
  vec2(-3, -3), vec2(-2, -3), vec2(-1, -3), vec2(+0, -3), vec2(+1, -3), vec2(+2, -3), vec2(+3, -3)
);
vec4 filter_7x7(sampler2D tex, float kernel[49]) {
  const int N = 49;
  ivec2 tex_size = textureSize(tex, 0);
  vec2 dxdy = 1.0 / vec2(tex_size);
  vec4 sampled[N];
  for(int i = 0; i < N; i++)
    sampled[i] = vec4(texture(tex, TexCoord.xy + dxdy * kernel_i7x7[i]));
  vec4 color = vec4(0.0);
  float kernel_sum = 0.0;
  for(int i = 0; i < N; i++){
    color += sampled[i] * kernel[i];
    kernel_sum += kernel[i];
  }
  return color / kernel_sum;
}
void main() {
  const float Gaussian_7x7[49] = float[49](
    0.000036, 0.000363, 0.001446, 0.002291, 0.001446, 0.000363, 0.000036,
    0.000363, 0.003676, 0.014662, 0.023226, 0.014662, 0.003676, 0.000363,
    0.001446, 0.014662, 0.058488, 0.092651, 0.058488, 0.014662, 0.001446,
    0.002291, 0.023226, 0.092651, 0.146768, 0.092651, 0.023226, 0.002291,
    0.001446, 0.014662, 0.058488, 0.092651, 0.058488, 0.014662, 0.001446,
    0.000363, 0.003676, 0.014662, 0.023226, 0.014662, 0.003676, 0.000363,
    0.000036, 0.000363, 0.001446, 0.002291, 0.001446, 0.000363, 0.000036
  );
  vec2 color = filter_7x7(tex0, Gaussian_7x7).rg;
  FragColor = vec4(vec2(color), 1.0, 1.0) * vec4(ColorMask, 1.0);
}
