#include "sgl_pass.h"

namespace sgl {

/*
A global sprite renderer instance.
*/
SpriteRenderer sprite_renderer;

Mat4x4 EyeParams::get_view_matrix() const {
  return sgl::get_view_matrix(eye.position, eye.look_at, eye.up_dir);
}

Mat4x4 EyeParams::get_projection_matrix(int w, int h) const {
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
EyeParams::EyeParams()
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

AnimatedModelRenderer::AnimatedModelRenderer() { 
  play_time = 0.0; 
}

AnimatedModelRenderer::VS_IN AnimatedModelRenderer::_convert_from_mesh_vertex(const Vertex_pnt_nm_bone & v) const
{
  VS_IN v0;
  v0.p = v.position;
  v0.n = v.normal;
  v0.t = v.texcoord;
  v0.bone_IDs = v.bone_IDs;
  v0.bone_weights = v.bone_weights;
  return v0;
}

void AnimatedModelRenderer::draw() {

  /* setup uniforms */
  this->uniforms.world = this->model.get_model_transform();
  this->uniforms.view = this->get_view_matrix();
  this->uniforms.projection = this->get_projection_matrix(this->pipeline.get_render_target(0)->get_width(), this->pipeline.get_render_target(0)->get_height());

  /* Rendering all the mesh parts in model */
  const std::vector<Mesh>& mesh_data = model.get_meshes();
  const std::vector<Material>& materials = model.get_materials();

  Timer timer;
  timer.tick();

  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    const int32_t mat_id = mesh_data[i_mesh].mat_id;
    const Mesh& mesh = mesh_data[i_mesh];
    /* calculate bone tranformation matrices and update uniform variables */
    this->model.update_skeletal_animation_for_mesh(mesh, anim_name, play_time, uniforms.bone_matrices);
    /* Setting up mesh materials. */
    this->uniforms.in_textures[0] = &materials[mat_id].diffuse_texture; /* diffuse texture */
    /* Launch the pipeline to render all the triangles in this mesh */
    this->pipeline.draw(shader, vertices_map[i_mesh], indices_map[i_mesh], uniforms);
  }

  this->last_draw_time = timer.tick();
}

void AnimatedModelRenderer::load_model_zip(const std::string & zip_file, const std::string& model_fname)
{
  this->vertices_map.clear();
  this->indices_map.clear();

  this->model.load_zip(zip_file, model_fname);

  /* load mesh data */
  const std::vector<Mesh>& mesh_data = model.get_meshes();
  const std::vector<Material>& materials = model.get_materials();

  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    const std::vector<Vertex_pnt_nm_bone>& vertices = mesh_data[i_mesh].vertices;
    const std::vector<int32_t>& indices = mesh_data[i_mesh].indices;
    /* load vertices */
    this->vertices_map.insert(std::pair<uint32_t, Pipeline_t::VertexBuffer_t>(i_mesh, Pipeline_t::VertexBuffer_t()));
    for (uint32_t i_vert=0; i_vert < vertices.size(); i_vert++) {
      this->vertices_map[i_mesh].push_back(_convert_from_mesh_vertex(vertices[i_vert]));
    }
    /* load indices */
    this->indices_map.insert(std::pair<uint32_t, Pipeline_t::IndexBuffer_t>(i_mesh, Pipeline_t::IndexBuffer_t()));
    for (uint32_t i_ind=0; i_ind < indices.size(); i_ind++) {
      this->indices_map[i_mesh].push_back(indices[i_ind]);
    }
  }
}

void AnimatedModelRenderer::clear_pipeline_cache()
{
  this->pipeline.clear_cache(); 
}

void AnimatedModelRenderer::clear_render_targets(const Vec4& clear_color)
{
  this->pipeline.clear_render_targets(clear_color);
}

void AnimatedModelRenderer::play_animation(const std::string& anim_name, const double& play_time)
{
  this->anim_name = anim_name; this->play_time = play_time; 
}

PipelineDrawMode AnimatedModelRenderer::get_draw_mode() const 
{ 
  return this->pipeline.get_draw_mode();
}

void AnimatedModelRenderer::set_draw_mode(PipelineDrawMode draw_mode)
{
  this->pipeline.set_draw_mode(draw_mode);
}

bool AnimatedModelRenderer::get_backface_culling_state() const
{
  return this->pipeline.get_backface_culling_state();
}

void AnimatedModelRenderer::set_backface_culling_state(bool state)
{
  this->pipeline.set_backface_culling_state(state);
}

void AnimatedModelRenderer::set_pipeline_num_threads(int num_threads)
{
  this->pipeline.set_num_threads(num_threads);
}

double AnimatedModelRenderer::query_last_draw_time() const
{
  return this->last_draw_time;
}

void AnimatedModelRenderer::bind_render_targets(Texture* color, Texture* depth, Texture* normal) {
  this->pipeline.bind_render_target(0, color);
  this->pipeline.bind_render_target(1, depth);
  this->pipeline.bind_render_target(2, normal);
}

void AnimatedModelRenderer::set_model_transform(const Mat4x4 & transform)
{
  this->model.set_model_transform(transform);
}

inline void AnimatedModelRenderer::VS_OUT::operator*=(const double& scalar)
{
  this->gl_Position *= scalar;
  this->wp *= scalar;
  this->wn *= scalar;
  this->t *= scalar;
}

inline AnimatedModelRenderer::VS_OUT AnimatedModelRenderer::VS_OUT::operator*(const double& scalar) const {
  VS_OUT out;
  out.gl_Position = this->gl_Position * scalar;
  out.wp = this->wp * scalar;
  out.wn = this->wn * scalar;
  out.t = this->t * scalar;
  return out;
}

inline AnimatedModelRenderer::VS_OUT AnimatedModelRenderer::VS_OUT::operator+(const VS_OUT& frag) const {
  VS_OUT out;
  out.gl_Position = this->gl_Position + frag.gl_Position;
  out.wp = this->wp + frag.wp;
  out.wn = this->wn + frag.wn;
  out.t = this->t + frag.t;
  return out;
}

inline void AnimatedModelRenderer::Shader::VS(const Uniforms & uniforms, const VS_IN & vertex_in, VS_OUT & vertex_out, Vec4 & gl_Position) const
{
  /* uniforms:
  * in_textures[0]: diffuse texture.
  * */
  const Mat4x4 &world = uniforms.world;
  const Mat4x4 &view = uniforms.view;
  const Mat4x4 &projection = uniforms.projection;
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
    for (uint32_t i_bone = 0; i_bone < MAX_BONES_INFLUENCE_PER_VERTEX; i_bone++)
    {
      int32_t bone_id = vertex_in.bone_IDs.i[i_bone];
      /* bone_id can be negative, which indicates that the
      * corresponding slot is unused. */
      if (bone_id < 0) break;
      double bone_weight = vertex_in.bone_weights.i[i_bone];
      const Mat4x4& bone_matrix = uniforms.bone_matrices[bone_id];
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

inline void AnimatedModelRenderer::Shader::FS(const Uniforms & uniforms, const FS_IN& fragment_in,
  const Vec4 & gl_FragCoord, FS_Outputs & fs_outs, bool & discard, double & gl_FragDepth) const
{
  Vec2 uv = Vec2(fragment_in.t.x, fragment_in.t.y);
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  Vec3 wn = fragment_in.wn;
  Vec3 wp = fragment_in.wp;
  double falloff = dot(wn, Vec3(0, 1, 0));
  falloff = (falloff + 1) * 0.5;
  fs_outs[0] = Vec4(textured * falloff, 1.0);
  fs_outs[2] = Vec4((wn + 1.0)*0.5, 1.0);
}

void blit_texture(sgl::Texture * source, sgl::Texture * target, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, const Vec2 & scale, const double & rot, const Vec3 & color_mask, const SpriteOriginMode origin_mode, const Texture * src_mask)
{
  sprite_renderer.bind_render_target(target);
  sprite_renderer.set_num_threads(2);
  sprite_renderer.set_sprite_origin_mode(origin_mode);
  sprite_renderer.draw(source, src_x, src_y, src_w, src_h, dst_x, dst_y, scale, rot, color_mask, src_mask);
}

}; /* namespace sgl */
