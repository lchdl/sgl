#pragma once

#include "sgl_pipeline.h"

namespace sgl {

Mat4x4 get_view_matrix(Vec3 eye, Vec3 look_at, Vec3 up);
Mat4x4 get_perspective_matrix(double aspect_ratio, double near, double far, double field_of_view);
Mat4x4 get_orthographic_matrix(double near, double far, double width, double height);

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
Simply draw a model (probably with animation) onto screen.
* Note that a model can consists of multiple meshes, so this class also 
  wraps up multiple draw calls to fully render a model, each draw call
  only renders a single mesh.
**/
class BaseAnimator : public Pass {
public:
  /* note: not owned */
  struct {
    Texture* color;
    Texture* depth;
    Texture* normal;
  } out_texs;
  struct {
    VS_func_t VS;
    FS_func_t FS;
  } shaders;
  Uniforms uniforms;

public:
  /* note: class instances not owned */
  Pipeline*    pipeline; /* the pipeline that is used to render to model */
  Model*          model; /* a pointer to model object that is being drawn */
  std::string anim_name; /* name of the current animation being played */
  double      play_time; /* time value for controlling the skeletal animation (in sec.) */

public:
  /* performance statistics */
  double last_draw_time; /* draw time (sec) of the last frame */

public:
  bool validate() const;
  void run(bool clear=true);

  BaseAnimator();
  virtual ~BaseAnimator() {}
};
void BaseAnimator_VS(const Uniforms* uniforms, const Vertex& vertex_in, Vertex_gl& vertex_out);
void BaseAnimator_FS(const Uniforms* uniforms, const Fragment_gl& fragment_in, FS_Outputs& fs_outs, bool& is_discarded, double& gl_FragDepth);

class BaseSpriteRenderer : public Pass {
  /* TODO: add implementations for rendering sprites here */
};


}; /* namespace sgl */