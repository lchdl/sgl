#include "sgl_shader.h"

namespace sgl {

Mat4x4
get_view_matrix(const RenderConfig &render_config) {
  Vec3 front =
      normalize(render_config.eye.position - render_config.eye.look_at);
  Vec3 left = normalize(cross(render_config.eye.up_dir, front));
  Vec3 up = normalize(cross(front, left));
  Vec3 &F = front, &L = left, &U = up;
  const double &ex = render_config.eye.position.x,
               &ey = render_config.eye.position.y,
               &ez = render_config.eye.position.z;
  Mat4x4 rotation(L.x, L.y, L.z, 0.0, U.x, U.y, U.z, 0.0, F.x, F.y, F.z, 0.0,
                  0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(1.0, 0.0, 0.0, -ex, 0.0, 1.0, 0.0, -ey, 0.0, 0.0, 1.0, -ez,
                     0.0, 0.0, 0.0, 1.0);
  Mat4x4 view_matrix = mul(rotation, translation);
  return view_matrix;
}

Mat4x4
get_projection_matrix(const RenderConfig &render_config) {
  Mat4x4 projection_matrix;
  Texture *color_texture =
      render_config.textures[render_config.color_texture_id];
  if (render_config.eye.perspective.enabled) {
    double aspect_ratio = double(color_texture->w) / double(color_texture->h);
    double inv_aspect = double(1.0) / aspect_ratio;
    double near = render_config.eye.perspective.near;
    double far = render_config.eye.perspective.far;
    double field_of_view = render_config.eye.perspective.field_of_view;
    double left = -tan(field_of_view / double(2.0)) * near;
    double right = -left;
    double top = inv_aspect * right;
    double bottom = -top;
    projection_matrix =
        Mat4x4(2 * near / (right - left), 0, (right + left) / (right - left), 0,
               0, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0,
               0, 0, -(far + near) / (far - near),
               -2 * far * near / (far - near), 0, 0, -1.0, 0);
    return projection_matrix;
  } else {
    /* not implemented now */
    return projection_matrix;
  }
}

void
prepare_uniforms(const RenderConfig &render_config, Uniforms &uniforms) {
  uniforms.model = render_config.model_transform;
  uniforms.view = get_view_matrix(render_config);
  uniforms.projection = get_projection_matrix(render_config);
  for (int i = 0; i < MAX_TEXTURES_PER_SHADING_UNIT; i++) {
    uniforms.in_textures[i] =
        render_config.textures[render_config.uniform_texture_ids[i]];
  }
}

void
vertex_shader(const Vertex &vertex_in, const Uniforms &uniforms,
              Vertex_gl &vertex_out) {
  const Mat4x4 &model = uniforms.model;
  const Mat4x4 &view = uniforms.view;
  const Mat4x4 &projection = uniforms.projection;
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
fragment_shader(const Fragment_gl &fragment_in, const Uniforms &uniforms,
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

void
fragment_shader2(const Fragment_gl &fragment_in, const Uniforms &uniforms,
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
	fragment_out = Vec4(color, 1.0);
}



}; // namespace sgl
