#include "sgl_pass.h"

namespace sgl {

Mat4x4 get_view_matrix(Vec3 eye, Vec3 look_at, Vec3 up)
{
  Vec3 front = normalize(eye - look_at);
  Vec3 left = normalize(cross(up, front));
  Vec3 up0 = normalize(cross(front, left));
  Vec3 &F = front, &L = left, &U = up0;
  const double &ex = eye.x, &ey = eye.y, &ez = eye.z;
  Mat4x4 rotation(
    L.x, L.y, L.z, 0.0,
    U.x, U.y, U.z, 0.0,
    F.x, F.y, F.z, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(
    1.0, 0.0, 0.0, -ex,
    0.0, 1.0, 0.0, -ey,
    0.0, 0.0, 1.0, -ez,
    0.0, 0.0, 0.0, 1.0);
  return mul(rotation, translation);
}
Mat4x4 get_perspective_matrix(double aspect_ratio, double near, double far, double field_of_view) {
  /* aspect_ratio = w/h */
  double inv_aspect = double(1.0) / aspect_ratio;
  double& n = near; /* near */
  double& f = far; /* far */
  double& fov = field_of_view;
  double l = -tan(fov / double(2.0)) * n; /* left */
  double r = -l; /* right */
  double t = inv_aspect * r; /* top */
  double b = -t; /* bottom */
  return Mat4x4(
    2 * n / (r - l), 0.0, (r + l) / (r - l), 0.0,
    0.0, 2 * n / (t - b), (t + b) / (t - b), 0.0,
    0.0, 0.0, -(f + n) / (f - n), -2 * f * n / (f - n),
    0.0, 0.0, -1.0, 0.0);
}
Mat4x4 get_orthographic_matrix(double near, double far, double width, double height) {
  double& n = near;
  double& f = far;
  double r = width * 0.5;
  double l = -r;
  double t = height * 0.5;
  double b = -t;
  return Mat4x4(
    2.0 / (r - l), 0.0, 0.0, -(r + l) / (r - l),
    0.0, 2.0 / (t - b), 0.0, -(t + b) / (t - b),
    0.0, 0.0, -2.0 / (f - n), -(f + n) / (f - n),
    0.0, 0.0, 0.0, 1.0
  );
}

Mat4x4 Pass::get_view_matrix() const {
  return sgl::get_view_matrix(eye.position, eye.look_at, eye.up_dir);
}

Mat4x4 Pass::get_projection_matrix(int w, int h) const {
  Mat4x4 projection_matrix;
  if (eye.perspective.enabled) {
    double aspect_ratio = double(w) / double(h);
    return get_perspective_matrix(aspect_ratio, eye.perspective.near,
      eye.perspective.far, eye.perspective.field_of_view);
  }
  else {
    return get_orthographic_matrix(eye.orthographic.near, eye.orthographic.far,
      eye.orthographic.width, eye.orthographic.height);
  }
}
Pass::Pass()
{
  eye.look_at = Vec3(0, 0, 0);
  eye.position = Vec3(10, 10, 10);
  eye.up_dir = Vec3(0, 1, 0);
  eye.perspective.enabled = true;
  eye.perspective.near = 0.1;
  eye.perspective.far = 100.0;
  eye.perspective.field_of_view = PI / 4.0;
  eye.orthographic.enabled = false;
  eye.orthographic.width = 256.0;
  eye.orthographic.height = 256.0;
  eye.orthographic.near = 0.1;
  eye.orthographic.far = 100.0;
}

BaseAnimator::BaseAnimator() { 
  shaders.VS = NULL;
  shaders.FS = NULL;
  model = NULL; 
  play_time = 0.0; 
  pipeline = NULL;
  out_texs.color = NULL;
  out_texs.depth = NULL;
  out_texs.normal = NULL;
}

double
BaseAnimator::run(bool clear) {
  if (this->model == NULL) return 0.0;

  this->pipeline->set_shaders(shaders.VS, shaders.FS);
  this->pipeline->set_render_target(0, out_texs.color);
  this->pipeline->set_render_target(1, out_texs.depth);
  this->pipeline->set_render_target(2, out_texs.normal);
  if (clear)
    this->pipeline->clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));

  /* setup internal variables (gl_*) */
  if (this->eye.perspective.enabled) {
    uniforms.gl_DepthRange.x = this->eye.perspective.near;
    uniforms.gl_DepthRange.y = this->eye.perspective.far;
    uniforms.gl_DepthRange.z = uniforms.gl_DepthRange.y - uniforms.gl_DepthRange.x;
  }
  else {
    uniforms.gl_DepthRange.x = this->eye.orthographic.near;
    uniforms.gl_DepthRange.y = this->eye.orthographic.far;
    uniforms.gl_DepthRange.z = uniforms.gl_DepthRange.y - uniforms.gl_DepthRange.x;
  }
  uniforms.model = this->model->get_model_transform();
  uniforms.view = this->get_view_matrix();
  uniforms.projection = this->get_projection_matrix(out_texs.color->w, out_texs.color->h);

  /* Rendering all the mesh parts in model */
  const std::vector<Mesh>& mesh_data = model->get_meshes();
  const std::vector<Material>& materials = model->get_materials();

  Timer timer;
  timer.tick();

  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    const VertexBuffer_t& vertices = mesh_data[i_mesh].vertices;
    const IndexBuffer_t& indices = mesh_data[i_mesh].indices;
    const int32_t mat_id = mesh_data[i_mesh].mat_id;
    const Mesh& mesh = mesh_data[i_mesh];

    /* calculate bone tranformation matrices and update uniform variables */
    this->model->update_skeletal_animation_for_mesh(mesh, anim_name, play_time, uniforms);
    /* Setting up mesh materials. */
    uniforms.in_textures[0] = &materials[mat_id].diffuse_texture; /* diffuse texture */
    /* Launch the pipeline to render all the triangles in this mesh */
    this->pipeline->draw(vertices, indices, uniforms);
  }

  return timer.tick();
}


}; /* namespace sgl */
