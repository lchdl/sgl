#include "sgl_shader.h"

namespace sgl {

void
VS_default(const Vertex &vertex_in, const Uniforms &uniforms,
              Vertex_gl &vertex_out) {
  /* Implement default vertex shader with bones and 
   * animation support. */
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
FS_default(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                Vec4 &fragment_out, bool& discard) {
	// int ix = fragment_in.gl_FragCoord.x;
	// int iy = fragment_in.gl_FragCoord.y;
	// if ((ix + iy) % 2 == 0) {
	// 	discard=true;
	// 	return;
	// }
  Vec2 uv = fragment_in.t;
  Vec3 wp = fragment_in.wp;
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  Vec3 color = (wp + 2.0) / 3.0;
  fragment_out = Vec4(color * textured, 1.0);
}

}; // namespace sgl
