#version 330 core
layout(location = 0) out vec4 FragVSM;
void main() {
  float depth = gl_FragCoord.z;
  /* 
  To fight with shadow acne, instead of directly set
                moment2 = depth * depth,
  we do this approximation instead:
  */
  float dx = dFdx(depth), dy = dFdy(depth);
  float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);
  /*
  Then write it back to depth texture.
  */
  FragVSM = vec4(depth, moment2, 0.0, 1.0);
}
