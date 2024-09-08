#include <stdio.h>
#include "sgl.h"

using namespace sgl;

struct Uniforms {
  Mat4x4 model, view, proj;
  const Texture *diffuse;
};

struct VS_IN : public IVertex {
  Vec3 p; /* position */
  Vec2 t; /* texcoords */

  VS_IN() {}
  VS_IN(Vec3 p, Vec2 t) { this->p = p; this->t = t; }
};

typedef struct VS_OUT : public IFragment {
  Vec3 wp; /* world position */
  Vec2 t; /* texcoords */

  VS_OUT() {}
  VS_OUT(Vec4 gl_Position, Vec3 wp, Vec2 t) { this->gl_Position = gl_Position; this->wp = wp; this->t = t; }
  void operator*=(const double& scalar) {
    this->wp *= scalar; this->t *= scalar; this->gl_Position *= scalar;
  }
  VS_OUT operator*(const double& scalar) const {
    return VS_OUT(this->gl_Position * scalar, this->wp * scalar, this->t * scalar);
  }
  VS_OUT operator+(const VS_OUT& frag) const {
    return VS_OUT(this->gl_Position + frag.gl_Position, this->wp + frag.wp, this->t + frag.t);
  }
} FS_IN;

struct Shader {
  void VS(const Uniforms& uniforms, const VS_IN& vertex_in, VS_OUT& vertex_out, Vec4& gl_Position) const {
    Mat4x4 transform = mul(mul(uniforms.proj, uniforms.view), uniforms.model);
    gl_Position = mul(transform, Vec4(vertex_in.p, 1.0));
    vertex_out.t = vertex_in.t;
    vertex_out.wp = mul(uniforms.model, Vec4(vertex_in.p, 1.0)).xyz();
  }
  void FS(const Uniforms& uniforms, const FS_IN& fragment_in, const Vec4& gl_FragCoord, FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const {
    Vec2 uv = fragment_in.t;
    Vec3 textured = texture(uniforms.diffuse, uv).rgb();
    fs_outs[0] = Vec4(textured, 1.0);
  }
};

void render_and_save_to_disk() {

  /* Step 1: Prepare resources */
  Texture color_texture, depth_texture, image_texture;
  const int width = 800, height = 600;
  color_texture = sgl::create_texture(width, height, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  depth_texture = sgl::create_texture(width, height, PixelFormat_Float64, TextureSampling_Nearest, TextureUsage_DepthBuffer);
  image_texture = sgl::load_texture("assets/tests/textures/checker_256.png", PixelFormat_BGRA8888);

  /* Step 2: Initialize render pipeline */
  Shader shader;
  Pipeline<Uniforms, VS_IN, FS_IN, Shader> pipeline;
  pipeline.bind_render_target(0, &color_texture);
  pipeline.bind_render_target(1, &depth_texture);
  pipeline.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.set_backface_culling_state(false);

  /* Step 3: Setup uniform variables */
  Uniforms uniforms;
  uniforms.model = Mat4x4(quat_to_mat3x3(Quat::rot_x(degrees_to_radians(-55.0)))); /* rotate model along x axis by -55 degrees */
  uniforms.view  = Mat4x4::translate(0.0, 0.0, -3.0); /* translate model along z axis by -3 units */
  uniforms.proj  = get_perspective_matrix(double(width) / double(height), 0.1, 10.0, degrees_to_radians(45)); /* near = 0.1, far = 10.0, field of view = 45 degrees */
  uniforms.diffuse = &image_texture;

  /* Step 4: Initialize vertices and triangles */
  Pipeline<Uniforms, VS_IN, FS_IN, Shader>::VertexBuffer_t vertices;
  vertices.resize(4);
  vertices[0] = VS_IN(Vec3(+0.5, +0.5, 0.0), Vec2(1.0, 1.0)); /* 1st vertex */
  vertices[1] = VS_IN(Vec3(+0.5, -0.5, 0.0), Vec2(1.0, 0.0)); /* 2nd vertex */
  vertices[2] = VS_IN(Vec3(-0.5, -0.5, 0.0), Vec2(0.0, 0.0)); /* 3rd vertex */
  vertices[3] = VS_IN(Vec3(-0.5, +0.5, 0.0), Vec2(0.0, 1.0)); /* 4th vertex */
  Pipeline<Uniforms, VS_IN, FS_IN, Shader>::IndexBuffer_t indices;
  indices.resize(6);
  indices[0] = 0; indices[1] = 1; indices[2] = 3; /* 1st triangle */
  indices[3] = 1; indices[4] = 2; indices[5] = 3; /* 2nd triangle */

  /* Step 5: Draw and save to disk */
  pipeline.draw(shader, vertices, indices, uniforms);
  color_texture.save_png("test_hello_world.png");
}

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */
  render_and_save_to_disk();
  return 0;
}
