#include <stdio.h>
#include "sgl.h"

using namespace sgl;

int w = 800, h = 600;

Pipeline pipeline;
VertexBuffer_t vertices;
IndexBuffer_t indices;

Texture color_texture, depth_texture, image_texture;

struct MyUniforms {
  Mat4x4 model, view, projection;
  const Texture *diffuse;
} uniforms;

void vertex_shader(const void *uniforms_data, const Vertex &vertex_in, Vertex_gl &vertex_out, Vec4& gl_Position) {
  const MyUniforms* uniforms = (const MyUniforms*)uniforms_data;
  const Mat4x4 &model = uniforms->model;
  const Mat4x4 &view = uniforms->view;
  const Mat4x4 &projection = uniforms->projection;
  Mat4x4 transform = mul(mul(projection, view), model);
  gl_Position = mul(transform, Vec4(vertex_in.p, 1.0));
  vertex_out.t = vertex_in.t;
  vertex_out.wn = mul(model, Vec4(vertex_in.n, 1.0)).xyz();
  vertex_out.wp = mul(model, Vec4(vertex_in.p, 1.0)).xyz();
}

void fragment_shader(const void *data, const Fragment_gl &fragment_in, const Vec4& gl_FragCoord, FS_Outputs &fs_outs,
  bool& is_discarded, double& gl_FragDepth) {
  const MyUniforms* uniforms = (const MyUniforms*)data;
  Vec2 uv = fragment_in.t;
  Vec3 textured = texture(uniforms->diffuse, uv).rgb();
  fs_outs[0] = Vec4(textured, 1.0);
}

void init_render() {
  /* create or load existing textures */
  color_texture = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  depth_texture = sgl::create_texture(w, h, PixelFormat_Float64, TextureSampling_Nearest, TextureUsage_DepthBuffer);
  image_texture = sgl::load_texture("textures/checker_256.png", PixelFormat_BGRA8888);

  /* set uniform variables */
  uniforms.model = quat_to_mat3x3(Quat::rot_x(degrees_to_radians(-55.0))); /* rotate model along x axis by -55 degrees */
  uniforms.view = Mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, -3, 0, 0, 0, 1); /* translate model along z axis by -3 units */
  uniforms.projection = get_perspective_matrix(double(w) / double(h), 0.1, 10.0, degrees_to_radians(45)); /* near = 0.1, far = 10.0, field of view = 45 degrees */
  uniforms.diffuse = &image_texture;

  /* initialize render pipeline */
  pipeline.bind_render_target(0, &color_texture);
  pipeline.bind_render_target(1, &depth_texture);
  pipeline.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.set_shaders(vertex_shader, fragment_shader);
  pipeline.disable_backface_culling();

  /* initialize vertices */
  Vertex v;
  v.p = Vec3(0.5, 0.5, 0.0); v.t = Vec2(1.0, 1.0);
  vertices.push_back(v);
  v.p = Vec3(0.5, -0.5, 0.0); v.t = Vec2(1.0, 0.0);
  vertices.push_back(v);
  v.p = Vec3(-0.5, -0.5, 0.0); v.t = Vec2(0.0, 0.0);
  vertices.push_back(v);
  v.p = Vec3(-0.5, 0.5, 0.0); v.t = Vec2(0.0, 1.0);
  vertices.push_back(v);

  /* initialize triangles */
  indices.resize(6);
  indices[0] = 0; indices[1] = 1; indices[2] = 3; /* first triangle */
  indices[3] = 1; indices[4] = 2; indices[5] = 3; /* second triangle */
}

void render_and_save_to_disk() {
  pipeline.draw(vertices, indices, &uniforms);
  color_texture.save_png("test_hello_world.png");
}

int main(int argc, char* argv[]) {

  set_cwd(gd(argv[0]));  /* set current working directory */
  init_render();
  render_and_save_to_disk();

  return 0;
}
