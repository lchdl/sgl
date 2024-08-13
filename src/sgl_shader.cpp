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
  FS_Outputs &fs_outs,
  bool& is_discarded,
  double& gl_FragDepth
) {
  Vec2 uv = fragment_in.t;
  Vec3 textured = texture(uniforms.in_textures[0], uv).rgb();
  fs_outs.set(0, Vec4(textured, 1.0));
}

void FS_Outputs::reset()
{
  /* lazy reset */
  memset(this->set_flags, 0, sizeof(uint8_t)*MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS);
}

void FS_Outputs::set(const int & slot, const Vec4 & value)
{
  set_flags[slot]=1;
  out_comps[slot] = value;
}

Vec4 FS_Outputs::get(const int & slot) const
{
  return out_comps[slot];
}

uint8_t FS_Outputs::query(const int & slot) const
{
  return set_flags[slot];
}

void FS_Outputs::invalidate(const int & slot)
{
  set_flags[slot]=0;
}

FS_Outputs::FS_Outputs()
{
  reset();
}

}; /* namespace sgl */
