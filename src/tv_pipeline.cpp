#include "tv_pipeline_impl.h"
#include "tv_model_impl.h"
#include "tv_context.h"

_tv_ppl_executor _tv_ppl_exe;

void tv_pipeline_init(int n_ppls)
{
  _tv_ppl_exe.set_active_ppls(n_ppls);
}

void _tv_ppl_executor::set_active_ppls(tv_u32_t n_ppls)
{
  this->n_ppls = n_ppls;
}

void _tv_ppl_executor::ppl_init(const tv_context& context, const tv_mat4x4& world)
{
  this->context = &context; /* store current context */
  #pragma omp parallel num_threads(n_ppls)
  for(tv_u32_t i_ppl=0; i_ppl<n_ppls; i_ppl++){
    _tv_ppl& ppl = ppls[i_ppl];
    ppl.init(context, n_ppls);
    ppl.set_uniform_mat4x4("world", world);
    tv_mat4x4 view = ppl.uniforms.view;
    tv_mat4x4 proj = ppl.uniforms.proj;
    ppl.set_uniform_mat4x4("transform", proj & view & world);
  }
}
void _tv_ppl_executor::ppl_vertex_transform(const tv_mesh& mesh)
{
  /*
  Run vertex shader
  local model space -> world space -> view space -> clip space -> NDC space
  */
  std::vector<tv_vertex> v_NDC = mesh.v_buf; /* copy buffer */
  {
    tv_u32_t n_vert = mesh.v_buf.size();
    tv_u32_t n_vert_per_ppl = (n_vert / n_ppls) + 1;
    
    #pragma omp parallel num_threads(n_ppls)
    for (tv_u32_t i_ppl = 0; i_ppl < n_ppls; i_ppl++) {
      tv_u32_t vi_start = n_vert_per_ppl * i_ppl;
      tv_u32_t vi_end = n_vert_per_ppl * (i_ppl + 1);
      if (vi_end > n_vert) vi_end = n_vert;
      _tv_ppl& ppl = ppls[i_ppl];
      for (tv_u32_t i_vert=vi_start; i_vert<vi_end; i_vert++){
        tv_vertex v_in = mesh.v_buf[i_vert];
        tv_vertex v_out;
        /* run vertex shader here, and we expect `gl_Position` 
        is properly set after running this */
        ppl.vertex_shader(v_in, v_out);
        /* `v_out` is in clipped space, divide by `w_c` to obtain NDC 
        (normalized device coordinates) */
        v_out.p = tv_vec3(ppl.gl_Position.x, ppl.gl_Position.y, ppl.gl_Position.z) / ppl.gl_Position.w;
        v_NDC[i_vert] = v_out;
      }
    }
  }

  /*
  Backface culling & clipping & prepare for rasterization
  */
  std::vector<_tv_ppl_tri> t_frag_ppl[64];
  {
    tv_u32_t n_tris = mesh.i_buf.size() / 3;
    tv_u32_t n_tris_per_ppl = (n_tris / n_ppls) + 1;
    #pragma omp parallel num_threads(n_ppls)
    for (tv_u32_t i_ppl = 0; i_ppl < n_ppls; i_ppl++) {

      tv_u32_t i_tri_start = n_tris_per_ppl * i_ppl;
      tv_u32_t i_tri_end = n_tris_per_ppl * (i_ppl + 1);
      if (i_tri_end > n_tris) i_tri_end = n_tris;

      _tv_ppl& ppl = ppls[i_ppl];
      for (tv_u32_t i_tri = i_tri_start; i_tri < i_tri_end; i_tri++) {
        _tv_ppl_tri t_in_raw(mesh.v_buf[i_tri*3], mesh.v_buf[i_tri*3+1], mesh.v_buf[i_tri*3+2]);
        if (context->c_flag & TV_BACKFACE_CULLING_BIT) {
          tv_vec3 v0v1 = t_in_raw.v[1].p - t_in_raw.v[0].p;
          tv_vec3 v0v2 = t_in_raw.v[2].p - t_in_raw.v[0].p;
          tv_vec3 facedir = tv_cross(v0v1, v0v2);
          if (tv_dot(facedir, ppl.uniforms.eyedir) > tv_float(0.0))
            continue;
        }
        _tv_ppl_tri t_in_NDC(v_NDC[i_tri*3], v_NDC[i_tri*3+1], v_NDC[i_tri*3+2]);
        ppl.clip_triangle(t_in_NDC, ppl.frustum, t_frag_ppl[i_ppl]);
      }
      for(tv_u32_t i_tri = 0; i_tri<t_frag_ppl[i_ppl].size(); i_tri++){
        _tv_ppl_tri& tri = t_frag_ppl[i_ppl][i_tri];
        ppl.NDC_to_window(tri.v[0].p, tri.v[0].p);
        ppl.NDC_to_window(tri.v[1].p, tri.v[1].p);
        ppl.NDC_to_window(tri.v[2].p, tri.v[2].p);
      }
    }
  }
  /*
  rasterization
  */
  std::vector<_tv_ppl_tri> t_frag;
  {
    /* collect triangles in all pipelines */
    for (tv_u32_t i=0;i<64;i++)
      t_frag.insert(t_frag.end(), t_frag_ppl[i].begin(), t_frag_ppl[i].end());
    /* parallel: `i`-th scanline is handled by (`i`%`n_ppls`)-th pipeline (interlaced) */
    tv_u32_t n_tris = t_frag.size();
    #pragma omp parallel num_threads(n_ppls)
    for (tv_u32_t i_ppl = 0; i_ppl < n_ppls; i_ppl++) {
      /* NOTE: window origin is at lower left corner. */
      const tv_float y_step = tv_float(n_ppls);
      tv_float x_cur = 0.0, y_cur = tv_float(i_ppl);
      _tv_ppl& ppl = ppls[i_ppl];
      for (tv_u32_t i_tri = 0; i_tri < n_tris; i_tri++){
        /* rasterize single triangle */
      }
    }
  }
  

}

_tv_ppl::_tv_ppl()
{
}
_tv_ppl::~_tv_ppl()
{
}

void _tv_ppl::init(const tv_context& context, const tv_u32_t& n_ppls)
{
  /* view matrix */
  /*
  NOTE: OpenGL uses a fairly strange axis system, the +z axis
  is pointing towards the camera, +x axis is pointing to the right,
  +y axis is pointing upwards, in this way, +x, +y, and +z axis
  is in a RHS coordinate system.
  */
  tv_vec3 front = tv_normalize(context.eye - context.look);
  tv_vec3 left = tv_normalize(tv_cross(context.up, front));
  tv_vec3 up = tv_normalize(tv_cross(front, left));
  tv_vec3& F = front, &L = left, &U = up;
  const tv_float& ex = context.eye.x, 
                & ey = context.eye.y, 
                & ez = context.eye.z;
  tv_mat4x4 _rot(L.x, L.y, L.z, 0.0, 
                 U.x, U.y, U.z, 0.0, 
                 F.x, F.y, F.z, 0.0,
                 0.0, 0.0, 0.0, 1.0);
  tv_mat4x4 _off(1.0, 0.0, 0.0, -ex, 
                 0.0, 1.0, 0.0, -ey, 
                 0.0, 0.0, 1.0, -ez,
                 0.0, 0.0, 0.0, 1.0);  
  tv_mat4x4 _view = _rot & _off;
  this->uniforms.eyedir = -front;
  this->uniforms.view = _view;
  /* projection matrix */
  if (context.perspective.enabled) {
    /* build view frustum */
    tv_float aspect = tv_float(context.color_surface->w) /
                      tv_float(context.color_surface->h);
    tv_float inv_aspect = tv_float(1.0) / aspect;
    tv_float n = context.perspective.near;
    tv_float f = context.perspective.far;
    tv_float l =
        -tv_tan(context.perspective.field_of_view / tv_float(2.0)) * n;
    tv_float r = -l;
    tv_float t = inv_aspect * r;
    tv_float b = -t;
    tv_mat4x4 _proj(2 * n / (r - l), 0, (r + l) / (r - l), 0, 
                    0, 2 * n / (t - b), (t + b) / (t - b), 0, 
                    0, 0, -(f + n) / (f - n), -2 * f * n / (f - n), 
                    0, 0, -1.0, 0);
    this->uniforms.proj = _proj;
    _tv_view_frustum frustum;
    /* near plane */
    frustum.faces[0].origin = tv_vec3(0, 0, +1);
    frustum.faces[0].normal = tv_vec3(0, 0, -1);
    /* far plane */
    frustum.faces[1].origin = tv_vec3(0, 0, -1);
    frustum.faces[1].normal = tv_vec3(0, 0, +1);
    /* left plane */
    frustum.faces[2].origin = tv_vec3(-1, 0, 0);
    frustum.faces[2].normal = tv_vec3(+1, 0, 0);
    /* right plane */
    frustum.faces[3].origin = tv_vec3(+1, 0, 0);
    frustum.faces[3].normal = tv_vec3(-1, 0, 0);
    /* top plane */
    frustum.faces[4].origin = tv_vec3(0, +1, 0);
    frustum.faces[4].normal = tv_vec3(0, -1, 0);
    /* bottom plane */
    frustum.faces[5].origin = tv_vec3(0, -1, 0);
    frustum.faces[5].normal = tv_vec3(0, +1, 0);
    this->frustum = frustum;
  } else { /* context.orthographic.enabled */
           /* TODO: add implementation here */
  }

  /* fragment shader */
  this->uniforms.h = context.color_surface->h;
  this->uniforms.w = context.color_surface->w;
  this->uniforms.n_ppls = n_ppls;
}

void _tv_ppl::NDC_to_window(const tv_vec3& in, tv_vec3& out)
{
  out.x = (in.x + 1.0) / 2.0 * uniforms.w;
  out.y = (in.y + 1.0) / 2.0 * uniforms.h;
  out.z = (in.z + 1.0) / 2.0; /* see glDepthRange() */
}

void _tv_ppl::clip_segment(const tv_vec3& p1, const tv_vec3& p2, 
                           const tv_vec3& o, const tv_vec3& n, 
                           tv_vec3& q, tv_float& d, tv_float& r)
{
  tv_vec3 dir = tv_normalize(p2 - p1);
  tv_float proj = tv_dot(dir, n);
  tv_float h = tv_dot(o - p1, n);
  d = h / proj;
  q = p1 + d * dir;
  r = d / tv_length(p2 - p1);
}

void _tv_ppl::clip_triangle(const tv_vertex& v1, const tv_vertex& v2, const tv_vertex& v3,
                            const tv_vec3& o, const tv_vec3& n,
                            tv_vertex& q1, tv_vertex& q2, tv_vertex& q3, tv_vertex& q4,
                            tv_u32_t& n_tri)
{
  tv_i8_t p1_sign = tv_dot(v1.p - o, n) < 0 ? -1 : 1;
  tv_i8_t p2_sign = tv_dot(v2.p - o, n) < 0 ? -1 : 1;
  tv_i8_t p3_sign = tv_dot(v3.p - o, n) < 0 ? -1 : 1;

  if (p1_sign < 0 && p2_sign < 0 && p3_sign < 0) {
    /* all triangles are clipped out */
    n_tri = 0;
  } else if (p1_sign > 0 && p2_sign > 0 && p3_sign > 0) {
    /* all triangles are contained by the upper space of the
    clipping plane, we don't need to do any clipping operations */
    n_tri = 1;
    q1 = v1; q2 = v2; q3 = v3;
  } else {
    /* check how many vertices are in the upper side of the plane */
    n_tri = 0;
    if (p1_sign > 0) n_tri++;
    if (p2_sign > 0) n_tri++;
    if (p3_sign > 0) n_tri++;
    const tv_vertex *v[3];
    /* sort pointers */
    if (n_tri == 1) {
      if (p1_sign > 0) {
        v[0] = &v1; v[1] = &v2; v[2] = &v3;
      } else if (p2_sign > 0) {
        v[0] = &v2; v[1] = &v1; v[2] = &v3;
      } else {
        v[0] = &v3; v[1] = &v1; v[2] = &v2;
      }
    } else {
      if (p1_sign < 0) {
        v[0] = &v1; v[1] = &v2; v[2] = &v3;
      } else if (p2_sign < 0) {
        v[0] = &v2; v[1] = &v1; v[2] = &v3;
      } else {
        v[0] = &v3; v[1] = &v1; v[2] = &v2;
      }
    }
    tv_vec3 t[2];
    tv_float d[2], r[2];
    clip_segment(v[0]->p, v[1]->p, o, n, t[0], d[0], r[0]);
    clip_segment(v[0]->p, v[2]->p, o, n, t[1], d[1], r[1]);
    if (n_tri == 1) {
      q1 = *(v[0]);
    } else { /* n_tri==2 */
      q1 = *(v[1]);
      q4 = *(v[2]);
    }
    q2 = tv_lerp(*v[0], *v[1], r[0]);
    q3 = tv_lerp(*v[0], *v[2], r[1]);
  }
}

void _tv_ppl::clip_triangle(const _tv_ppl_tri& t_in,
                            const _tv_view_frustum& f,
                            std::vector<_tv_ppl_tri>& t_out)
{
  std::vector<_tv_ppl_tri> Q0, Q1;
  std::vector<_tv_ppl_tri> *Qcur = &Q0, *Qnext=&Q1, *Qtemp = NULL;
  Qcur->push_back(t_in);
  tv_vertex q[4];
  for (tv_u32_t i=0; i<6;i++){
    /* 
    for each face in the frustum `f`, clip all triangles in `Qcur` and store them to `Qnext`, 
    then swap `Qcur` and `Qnext` in the end of each iteration.
    */
    for (tv_u32_t j=0;j<Qcur->size();j++){
      _tv_ppl_tri& tri = Qcur->at(j);
      tv_u32_t n_tri;
      if (i==4) __debugbreak();
      clip_triangle(tri.v[0], tri.v[1], tri.v[2], 
                    f.faces[i].origin, f.faces[i].normal, 
                    q[0], q[1], q[2], q[3], n_tri);
      if (n_tri==1){
        Qnext->push_back(_tv_ppl_tri(q[0],q[1],q[2]));
      }
      else if (n_tri==2){
        Qnext->push_back(_tv_ppl_tri(q[0],q[1],q[2]));
        Qnext->push_back(_tv_ppl_tri(q[0],q[2],q[3]));
      }
    }
    /* swap `Qcur` and `Qnext` for the next iteration */
    Qcur->clear();
    Qtemp = Qcur;
    Qcur = Qnext;
    Qnext = Qtemp;
  }
  for(tv_u32_t i=0;i<Qcur->size();i++)
    t_out.push_back(Qcur->at(i));
}

void _tv_ppl::set_uniform_mat4x4(const std::string& name, const tv_mat4x4& m)
{
  if(name=="world") 
    this->uniforms.world = m;
  else if (name=="view")
    this->uniforms.view = m;
  else if (name=="proj")
    this->uniforms.proj = m;
  else if (name=="transform")
    this->uniforms.transform = m;
}

void _tv_ppl::vertex_shader(const tv_vertex& v_in, tv_vertex& v_out)
{
  tv_vec3 p_in = v_in.p;
  tv_vec4 p_out = uniforms.transform & tv_vec4(p_in, 1.0);
  gl_Position = p_out;
  v_out.n = v_in.n;
  v_out.t = v_in.t;
}

/* draw `mesh` using `context`, mesh transform is given in `world` matrix */
void tv_draw_mesh(tv_context* context, tv_mesh* mesh, const tv_mat4x4& world)
{
  _tv_ppl_exe.ppl_init(*context, world);
  _tv_ppl_exe.ppl_vertex_transform(*mesh);
}
