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
  /*
  Important note:
  Here, we write `mul(rotation, translation)` instead of `mul(translation, rotation)`. 
  The latter might seem reasonable since rotation is typically applied before translation. 
  However, we apply translation first, followed by rotation, as explained in detail at: 
  https://www.songho.ca/opengl/gl_camera.html.
  Note that the rotation matrix here is actually in its transposed (inverted) state, 
  so please don't be confused by the order of matrix multiplication.
  */
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

Mat4x4 get_orthographic_matrix(double near, double far, double left, double right, double top, double bottom) {
  double& n = near;
  double& f = far;
  double& r = right;
  double& l = left;
  double& t = top;
  double& b = bottom;
  return Mat4x4(
    2.0 / (r - l), 0.0, 0.0, -(r + l) / (r - l),
    0.0, 2.0 / (t - b), 0.0, -(t + b) / (t - b),
    0.0, 0.0, -2.0 / (f - n), -(f + n) / (f - n),
    0.0, 0.0, 0.0, 1.0
  );
}

Mat4x4 get_orthographic_matrix(double near, double far, double width, double height) {
  return get_orthographic_matrix(near, far, -width * 0.5, width * 0.5, height * 0.5, -height * 0.5);
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
  shaders.VS = BaseAnimator_VS;
  shaders.FS = BaseAnimator_FS;
  model = NULL; 
  play_time = 0.0; 
  pipeline = NULL;
  out_texs.color = NULL;
  out_texs.depth = NULL;
  out_texs.normal = NULL;
}

bool BaseAnimator::validate() const
{
  if (out_texs.color == NULL || out_texs.depth == NULL ||
    out_texs.normal == NULL) return false;
  if (shaders.VS == NULL || shaders.FS == NULL) return false;
  if (pipeline == NULL || model == NULL) return false;
  /* buffer sizes must be equal */
  if (out_texs.color->w != out_texs.depth->w || out_texs.depth->w != out_texs.normal->w)
    return false;
  if (out_texs.color->h != out_texs.depth->h || out_texs.depth->h != out_texs.normal->h)
    return false;
  return true;
}

void BaseAnimator::run(bool clear) {
  this->pipeline->set_shaders(shaders.VS, shaders.FS);
  this->pipeline->bind_render_target(0, out_texs.color);
  this->pipeline->bind_render_target(1, out_texs.depth);
  this->pipeline->bind_render_target(2, out_texs.normal);
  if (clear)
    this->pipeline->clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));

  /* setup uniforms */
  this->uniforms.world = this->model->get_model_transform();
  this->uniforms.view = this->get_view_matrix();
  this->uniforms.projection = this->get_projection_matrix(out_texs.color->w, out_texs.color->h);

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
    this->model->update_skeletal_animation_for_mesh(mesh, anim_name, play_time, uniforms.bone_matrices);
    /* Setting up mesh materials. */
    this->uniforms.in_textures[0] = &materials[mat_id].diffuse_texture; /* diffuse texture */
    /* Launch the pipeline to render all the triangles in this mesh */
    this->pipeline->draw(vertices, indices, &uniforms);
  }

  this->last_draw_time = timer.tick();
}

void BaseAnimator_VS(const void* uniforms_data, const Vertex& vertex_in, Vertex_gl& vertex_out, Vec4& gl_Position)
{
  const BaseAnimator_Uniforms* uniforms = (const BaseAnimator_Uniforms*)uniforms_data;
  /* uniforms:
   * in_textures[0]: diffuse texture.
   * */
  const Mat4x4 &world = uniforms->world;
  const Mat4x4 &view = uniforms->view;
  const Mat4x4 &projection = uniforms->projection;
  Mat4x4 transform_WVP = mul(projection, mul(view, world));

  if (vertex_in.bone_IDs.i[0] < 0) {
    /* vertex does not belong to any bone */
    gl_Position = mul(transform_WVP, Vec4(vertex_in.p, 1.0));
    vertex_out.t = vertex_in.t;
    vertex_out.wn = mul(world, Vec4(vertex_in.n, 1.0)).xyz();
    vertex_out.wp = mul(world, Vec4(vertex_in.p, 1.0)).xyz();
  }
  else {
    /* vertex is controlled by at least one bone */
    /* calculate:
     * p_final = sum( w[i]*m[i]*p, for i in [0,1,2,3] ), where
     * p is the vertex position in local model space (T-pose),
     * m[i] is the i-th final bone transformation matrix,
     * w[i] is the i-th bone influence weight to the vertex.
     * to make computation a little bit faster, we calculate
     * w[i]*m[i] for i in [0,1,2,3], then multiply it with p. */
    Mat4x4 bone_transform;
    for (uint32_t i_bone=0; i_bone < MAX_BONES_INFLUENCE_PER_VERTEX; i_bone++)
    {
      int32_t bone_id = vertex_in.bone_IDs.i[i_bone];
      /* bone_id can be negative, which indicates that the
       * corresponding slot is unused. */
      if (bone_id < 0) break;
      double bone_weight = vertex_in.bone_weights.i[i_bone];
      const Mat4x4& bone_matrix = uniforms->bone_matrices[bone_id];
      bone_transform += bone_weight * bone_matrix;
    }
    Vec4 p0 = mul(bone_transform, Vec4(vertex_in.p, 1.0));
    Vec4 n0 = mul(bone_transform, Vec4(vertex_in.n, 0.0));
    /* apply final matrix to vertex position */
    gl_Position = mul(transform_WVP, p0);
    /* copy texture coordinate */
    vertex_out.t = vertex_in.t;
    /* calculate world normal and position */
    vertex_out.wn = mul(world, n0).xyz();
    vertex_out.wn = normalize(vertex_out.wn);
    vertex_out.wp = mul(world, p0).xyz();
  }
}

void BaseAnimator_FS(const void* uniforms_data, const Fragment_gl& fragment_in, const Vec4& gl_FragCoord,
  FS_Outputs& fs_outs, bool& is_discarded, double& gl_FragDepth)
{
  const BaseAnimator_Uniforms* uniforms = (const BaseAnimator_Uniforms*)uniforms_data;

  Vec2 uv = Vec2(fragment_in.t.x, fragment_in.t.y);
  Vec3 textured = texture(uniforms->in_textures[0], uv).xyz();
  Vec3 wn = fragment_in.wn;
  Vec3 wp = fragment_in.wp;
  double falloff = dot(wn, Vec3(0, 1, 0));
  falloff = (falloff + 1) * 0.5;
  fs_outs[0] = Vec4(textured * falloff, 1.0);
  fs_outs[2] = Vec4((wn + 1.0)*0.5, 1.0);
}

BaseSpriteRenderer::BaseSpriteRenderer()
{
  Vertex v;
  v.p = Vec3(0.0, 0.0, 0.0); v.t = Vec2(0.0, 0.0);
  vertices.push_back(v);
  v.p = Vec3(1.0, 0.0, 0.0); v.t = Vec2(1.0, 0.0);
  vertices.push_back(v);
  v.p = Vec3(1.0, 1.0, 0.0); v.t = Vec2(1.0, 1.0);
  vertices.push_back(v);
  v.p = Vec3(0.0, 1.0, 0.0); v.t = Vec2(0.0, 1.0);
  vertices.push_back(v);
  indices.resize(6);
  indices[0] = 0; indices[1] = 1; indices[2] = 3;
  indices[3] = 1; indices[4] = 2; indices[5] = 3;
  out_texs.color = NULL;
}

void BaseSpriteRenderer_VS(const void* uniforms_data, const Vertex& vertex_in, Vertex_gl& vertex_out, Vec4& gl_Position)
{
  const BaseSpriteRenderer_Uniforms* uniforms = (const BaseSpriteRenderer_Uniforms*)uniforms_data;
  gl_Position = uniforms->transform * Vec4(vertex_in.p.xy(), 0.0, 1.0);
  vertex_out.t = vertex_in.t;
}
void BaseSpriteRenderer_FS(const void* uniforms_data, const Fragment_gl& fragment_in, const Vec4& gl_FragCoord, FS_Outputs& fs_outs, bool& is_discarded, double& gl_FragDepth)
{
  const BaseSpriteRenderer_Uniforms* uniforms = (const BaseSpriteRenderer_Uniforms*)uniforms_data;
  Vec4 tex_color = texture(uniforms->in_texture, fragment_in.t);
  if (tex_color.a < 0.99) { 
    is_discarded = true; 
    return;
  }
  fs_outs[0] = Vec4(uniforms->color_mask, 1.0) * tex_color;
}

void BaseSpriteRenderer::run(const Texture* tex, const Vec2& pos, const Vec2& scale, const double& rot, const Vec3& color_mask) {
  Mat4x4 scaling(
    scale.x, 0.0, 0.0, 0.0,
    0.0, scale.y, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(
    1.0, 0.0, 0.0, pos.x,
    0.0, 1.0, 0.0, pos.y,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 rotation(quat_to_mat3x3(Quat::rot_z(rot)));
  Mat4x4 model = translation * rotation * scaling;
  Mat4x4 projection = get_orthographic_matrix(0.0, 1.0, 0.0, out_texs.color->w, out_texs.color->h, 0.0);
  vertices[0].p = Vec3(-0.5*tex->w, -0.5*tex->h, 0.0);
  vertices[1].p = Vec3(+0.5*tex->w, -0.5*tex->h, 0.0);
  vertices[2].p = Vec3(+0.5*tex->w, +0.5*tex->h, 0.0);
  vertices[3].p = Vec3(-0.5*tex->w, +0.5*tex->h, 0.0);
  uniforms.transform = projection * model;
  uniforms.color_mask = color_mask;
  uniforms.in_texture = tex;
  pipeline->bind_render_target(0, out_texs.color);
  pipeline->disable_depth_test();
  pipeline->set_shaders(BaseSpriteRenderer_VS, BaseSpriteRenderer_FS);
  pipeline->draw(vertices, indices, &uniforms);
  pipeline->enable_depth_test();
}


}; /* namespace sgl */
