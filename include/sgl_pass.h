#pragma once

#include "sgl_pipeline.h"

namespace sgl {

/**

A `pass` is an object that describes a complete render operation
and stores all the resources used during rendering. All `pass`
objects & instances should inherit from `Pass` base class.

* The reason I introduce the concept of `pass` is that drawing an
object onto the screen correctly requires a lot of preparation
work beforehand, including but not limited to shader initialization,
buffer preparation, uniform variable assignment, etc. To be honest,
many things can go wrong here if not enough attention is paid, and
usually, a blank screen will be shown if there is any bug in your
code, which is not very informative for graphical debugging and can
lower your efficiency. So, wrapping the above process into a `pass`
can standardize the whole process for us, which will be much more
convenient when drawing something complex onto the screen.

**/

class Pass {
public:
  /* camera/eye settings */
  struct {
    Vec3 position; /* eye position */
    Vec3 look_at;  /* view target */
    Vec3 up_dir;   /* up normal */
    struct {
      bool enabled;
      double near, far, field_of_view;
    } perspective;
    struct {
      bool enabled;
      double near, far, width, height;
    } orthographic;
  } eye;
public:
  /* utility functions */
  Mat4x4 get_view_matrix() const;
  Mat4x4 get_projection_matrix(int w, int h) const;
  /* default ctor & dtor */
  Pass();
  virtual ~Pass() {}
};

/**
BaseAnimator:

Simply draw a model (probably with animation) onto screen.
* Note that a model can consists of multiple meshes, so this class also 
  wraps up multiple draw calls to fully render a model, each draw call
  only renders a single mesh.
**/

class BaseAnimator : public Pass
{
public:
  struct Uniforms {
    Mat4x4 world;
    Mat4x4 view;
    Mat4x4 projection;
    /* texture objects */
    const Texture *in_textures[MAX_TEXTURES_PER_SHADING_UNIT];
    /* final bone transformations */
    Mat4x4 bone_matrices[MAX_NODES_PER_MODEL];
  };
  struct Vertex : public IVertex {
    Vec3 p; /* vertex position (in model local space) */
    Vec3 n; /* vertex normal (in model local space)*/
    Vec2 t; /* vertex texture coordinate */
    /* for skeletal animations */
    IVec4 bone_IDs; /* bones up to 4 */
    Vec4  bone_weights;

    void convert_from(const MeshVertex& v);
  };
  struct Fragment : public IFragment {
    Vec3 wp; /* world position */
    Vec3 wn; /* world normal */
    Vec2 t;  /* texture coordinates */

    void    operator*=(const double& w);
    Fragment operator*(const double& w) const;
    Fragment operator+(const Fragment& frag) const;
  };
  class Shader {
  public:
    void VS(const Uniforms& uniforms, const Vertex& vertex_in, Fragment& vertex_out, Vec4& gl_Position) const;
    void FS(const Uniforms& uniforms, const Fragment& fragment_in, const Vec4& gl_FragCoord, FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const;
  };


public:
  void                        run(bool clear=true);
  void                 load_model(const std::string& file);
  PipelineDrawMode  get_draw_mode() const { return this->pipeline.get_draw_mode(); }
  void              set_draw_mode(PipelineDrawMode draw_mode) { this->pipeline.set_draw_mode(draw_mode); }
  bool get_backface_culling_state() const { return this->pipeline.get_backface_culling_state(); }
  void set_backface_culling_state(bool state) { this->pipeline.set_backface_culling_state(state); }
  void            set_num_threads(int num_threads) { this->pipeline.set_num_threads(num_threads); }
  void             play_animation(const std::string& anim_name, const double& play_time) { this->anim_name = anim_name; this->play_time = play_time; }
  double     query_last_draw_time() const { return this->last_draw_time; }
  void         set_render_targets(Texture* color, Texture* depth, Texture* normal) { this->out_texs.color=color; this->out_texs.depth=depth; this->out_texs.normal = normal; }

public:
  BaseAnimator();
  virtual ~BaseAnimator() {}

protected:
  struct {
    /* note: not owned */
    Texture* color;
    Texture* depth;
    Texture* normal;
  } out_texs;
  typedef Pipeline<Uniforms, Vertex, Fragment, Shader> Pipeline_t;
  Pipeline_t pipeline;
  Uniforms   uniforms;
  Shader       shader;
  Model         model;
  std::map<uint32_t, Pipeline_t::VertexBuffer_t> vertices_map;
  std::map<uint32_t, Pipeline_t::IndexBuffer_t>   indices_map;

  std::string anim_name; /* name of the current animation being played */
  double      play_time; /* time value for controlling the skeletal animation (in sec.) */
  double last_draw_time; /* draw time (sec) of the last frame */

};


}; /* namespace sgl */