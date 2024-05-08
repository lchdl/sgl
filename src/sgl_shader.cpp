#include "sgl_shader.h"

namespace sgl {

void
default_VS(
  const Uniforms &uniforms,
  const Vertex &vertex_in,
  Vertex_gl &vertex_out
) {
  /* Implement default vertex shader. */
  const Mat4x4 &model = uniforms.model;
  const Mat4x4 &view = uniforms.view;
  const Mat4x4 &projection = uniforms.projection;
  /* Model & View & Projection matrix */
  Mat4x4 transform = mul(mul(projection, view), model);
  Vec4 gl_Position = mul(transform, Vec4(vertex_in.p, 1.0));
  vertex_out.gl_Position = gl_Position;
  vertex_out.t = vertex_in.t;
  vertex_out.wn = mul(model, Vec4(vertex_in.n, 1.0)).xyz();
  vertex_out.wp = mul(model, Vec4(vertex_in.p, 1.0)).xyz();
}
void
assemble_fragment(const Vertex_gl &vertex_in, Fragment_gl &fragment_out) {
  fragment_out.wn = vertex_in.wn;
  fragment_out.wp = vertex_in.wp;
  fragment_out.t = vertex_in.t;
}
void
default_FS(
  const Uniforms &uniforms,
  const Fragment_gl &fragment_in,
  Vec4 &color_out,
  bool& is_discarded,
  double& gl_FragDepth
) {
  Vec2 uv = fragment_in.t;
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  color_out = Vec4(textured, 1.0);
}

}; /* namespace sgl */
