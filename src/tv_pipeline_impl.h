#pragma once
#include <vector>
#include <string>
#include "tv_pipeline.h"
#include "tv_model_impl.h"

struct _tv_view_frustum {
  struct {
    tv_vec3 origin, normal;
  } faces[6];
};

struct _tv_ppl_tri {
  /* internal triangle data structure used in fragment shader */
  tv_vertex v[3];

  /* ctor & dtor */
  _tv_ppl_tri(){}
  _tv_ppl_tri(const tv_vertex& v0, 
              const tv_vertex& v1, 
              const tv_vertex& v2)
  {
    this->v[0]=v0; this->v[1]=v1; this->v[2]=v2;
  }
};

class _tv_ppl 
{
  friend class _tv_ppl_executor;
public:
  /*
  Initialize vertex shader and fragment shader from `context`.
  */
  void init(const tv_context& context, const tv_u32_t& n_ppls);
  /*
  Set uniform variables.
  */
  void set_uniform_mat4x4(const std::string& name, const tv_mat4x4& m);
  /*
  clip a segment `p1`-`p2` using an infinite plane (origin:`o`, normal:`n`)
  their intersection is stored to `q`, 
  length of segment `p1`-`q` is stored to `d`,
  `r` stores len(`q`-`p1`) / len(`p2`-`p1`)
  */
  void clip_segment(const tv_vec3& p1, const tv_vec3& p2, 
                    const tv_vec3& o, const tv_vec3& n, 
                    tv_vec3& q, tv_float& d, tv_float& r);
  /*
  Clip a input triangle (`v1`,`v2`,`v3`) using an infinite plane (origin:`o`,
  normal:`n`), clipped results will be saved in (`q1`,`q2`,`q3`,`q4`).
  * If input triangle is completely outside the plane, then
    - `n_tri`=0.
  * If input triangle is completely inside the plane, then
    - `n_tri`=1, (`q1`,`q2`,`q3`) = (`v1`,`v2`,`v3`).
  * Otherwise, the input triangle will be clipped by given plane, the part that
    inside the plane will be kept, results in:
    - one triangle, `n_tri`=1, (`q1`,`q2`,`q3`)
    - two triangles, `n_tri`=2, (`q1`,`q2`,`q3`) and (`q1`,`q3`,`q4`).
  */
  void clip_triangle(const tv_vertex& v1, const tv_vertex& v2, const tv_vertex& v3, /* input triangle */
                     const tv_vec3& o, const tv_vec3& n,                            /* clipping plane */
                     tv_vertex& q1, tv_vertex& q2, tv_vertex& q3, tv_vertex& q4,    /* output triangle(s) */
                     tv_u32_t& n_tri /* number of output triangle(s) */
  );
  /*
  Clip a triangle `t_in` using frustum `f`. Store clipped triangle(s) to `t_out`.
  A frustum has 6 faces, each face is clipped with the triangle.
  */
  void clip_triangle(const _tv_ppl_tri& t_in,
                     const _tv_view_frustum& f,
                     std::vector<_tv_ppl_tri>& t_out
  );
  /*
  Main vertex shader process.
  Transform vertex `v_in` to `v_out`.
  - NOTE: this->`gl_Position` must be properly set after calling this function.
  */
  void vertex_shader(const tv_vertex& v_in, tv_vertex& v_out);
  /*
  convert NDC to screen coordinate
  x [-1, +1] ==> [0, w]
  y [-1, +1] ==> [h, 0]
  z [-1, +1] ==> [0, 1]
  */
  void NDC_to_window(const tv_vec3& in, tv_vec3& out);

  _tv_ppl();
  virtual ~_tv_ppl();

protected:
  /* clip space frustum */
  _tv_view_frustum frustum;

  /*
  Uniform variables used in vertex shader and fragment shader
  */
  struct{
    /* camera front vector */
    tv_vec3 eyedir;
    /* world, view, projection matrices */
    tv_mat4x4 world, view, proj;
    tv_mat4x4 transform; /* pre-calculated matrix */
    /* target canvas size */
    tv_u32_t h, w; 
    /* pipeline ID, number of concurrent pipelines */
    tv_u32_t i_ppl, n_ppls;
  } uniforms; /* fragment shader temoprary storage */

  /*
  internal variables
  */
  tv_vec4 gl_Position; /* should be properly set by vertex shader */
  tv_vec4 gl_FragCoord; /* can be used in fragment shader */
  tv_float gl_FragDepth; /* a value between [0, 1] */

}; /* supports 64 concurrent pipelines at maximum */

class _tv_ppl_executor
{
public:
  /*
  set number of active pipelines.
  this will affect 
  #pragma omp parallel num_threads(n_ppls)
  */
  void set_active_ppls(tv_u32_t n_ppls);
  /*
  get number of active pipelines.
  */
  tv_u32_t get_active_ppls() const;
  /*
  Initialize pipelines.
  */
  void ppl_init(const tv_context& context, const tv_mat4x4& world);
  /*
  activate vertex transform pipeline
  */
  void ppl_vertex_transform(const tv_mesh& mesh);


  _tv_ppl_executor(){context=NULL;}

protected:
  /* number of active concurrent processing pipelines */
  tv_u32_t n_ppls;
  _tv_ppl ppls[64];
  const tv_context* context; /* not owned */

};
