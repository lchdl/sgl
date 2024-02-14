#include "ppl_shader.h"

namespace ppl {

void
vertex_shader(const Vertex &vertex_in, const Uniforms &uniforms,
              Vertex_gl &vertex_out) {
  /* vertex shader */
  const Mat4x4 &model      = uniforms.model;
  const Mat4x4 &view       = uniforms.view;
  const Mat4x4 &projection = uniforms.projection;
  Mat4x4 transform         = mul(mul(projection, view), model);
  Vec4 gl_Position         = mul(transform, Vec4(vertex_in.p, 1.0));
  vertex_out.gl_Position   = gl_Position;
  vertex_out.p             = gl_Position.xyz();
  vertex_out.n             = mul(model, Vec4(vertex_in.n, 1.0)).xyz();
  vertex_out.t             = vertex_in.t;
}
void
fragment_shader(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                Vec4 &fragment_out) {
  Vec2 uv       = fragment_in.t;
  Vec4 textured = uniforms.in_texture->texture_RGBA8(uv);
  fragment_out  = Vec4(textured.xyz(), 1.0);
}
};   // namespace ppl
