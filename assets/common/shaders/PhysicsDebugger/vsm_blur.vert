#version 330 core
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
uniform ivec4 TexDims;  /* (Sw, Sh, Tw, Th) */
uniform vec4 Transform; /* (Sx, Sy, dx, dy) */
uniform float Rotation;
out vec2 TexCoord;

mat4x4 rotate_z(float angle) {
  float c = cos(angle), s = sin(angle);
  return mat4x4(c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
}
mat4x4 translate(float dx, float dy, float dz) {
  return mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, dx, dy, dz, 1);
}
mat4x4 scale(float sx, float sy, float sz) {
  return mat4x4(sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1);
}
mat4x4 ortho(float n, float f, float l, float r, float t, float b) {
  return mat4x4(2/(r-l), 0, 0, 0, 0, 2/(t-b), 0, 0, 0, 0, -2/(f-n), 0, 
    -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1);
}

void main() {
  mat4x4 mTranslate = translate(Transform.z, Transform.w, 0);
  mat4x4 mRotate = rotate_z(Rotation);
  mat4x4 mScale = scale(Transform.x, Transform.y, 1);
  mat4x4 mModel = mTranslate * mRotate * mScale;
  mat4x4 mProjection = ortho(0, 1, 0, TexDims.z, TexDims.w, 0);
  mat4x4 mTransform = mProjection * mModel;
  gl_Position = mTransform * vec4(TexDims.xy * inPosition.xy, 0, 1);
  TexCoord = inTexCoord;
}
