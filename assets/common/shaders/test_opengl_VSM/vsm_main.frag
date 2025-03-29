#version 330 core
layout(location = 0) out vec4 FragColor;
in vec2 TexCoord;
in vec4 LightSpacePos;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex_VSM;
float linstep(float low, float high, float v) {
  return clamp((v - low) / (high - low), 0.0, 1.0);
}
float sample_VSM(sampler2D tex_VSM, vec2 coords, float compare) {
  vec2 moments = texture(tex_VSM, coords.xy).xy;
  float p = step(compare, moments.x);
  /* Compute variance, but be aware of 0, so set it to a small epsilon. */
  float variance = max(moments.y - moments.x * moments.x, 0.00002);
  /* Implement Chebyshev's inequality: the maximum percentage of value. */
  float d = compare - moments.x;
  float p_max = variance / (variance + d * d);
  /* A simple hack to solve light bleeding brought by VSM technique. */
  p_max = linstep(0.2, 1.0, p_max); /* 0.2 empirically determined */
  /* If completely in light, p=1.0, then we don't need to return p_max, */
  /* which is slightly below 1.0, and we need to ensure p_max<1. */
  return min(max(p, p_max), 1.0);
}
float shadow_VSM(sampler2D tex_VSM, vec4 LightSpacePos) {
  /* Manually perform perspective divide and normalize coordinates to [0, 1]. */
  vec3 ShadowCoords = (LightSpacePos.xyz / LightSpacePos.w) * 0.5 + 0.5;
  return sample_VSM(tex_VSM, ShadowCoords.xy, ShadowCoords.z);
}
void main() {
  float ShadowAmount = shadow_VSM(tex_VSM, LightSpacePos);
  vec4 ShadowCoeff = vec4(vec3(clamp(ShadowAmount, 0.5, 1.0)), 1.0);	
  FragColor = ShadowCoeff * mix(texture(tex1, TexCoord), texture(tex2, TexCoord), 0.5);
}
