#pragma once

#include "sgl_physics/sgl_physics_solver.h"

/* if opengl is enabled, we provide a debugger implementation for physics engine debugging. */
#ifdef ENABLE_OPENGL
#include "sgl_OpenGL.h"
#include <map>
#include <vector>

namespace sgl {
namespace Physics {

class DebuggerCachedData {
  friend class Debugger;
public:
  struct Vertex {
    float position[3];
    float   normal[3];
    float texcoord[2];
  };
protected:
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_3f3f2f> conv_hull_vbuf;
  sgl::OpenGL::AnimatedModelRenderer renderer;
  sgl::Model* model;
public:
  DebuggerCachedData();
  virtual ~DebuggerCachedData();
};

typedef void(*DebuggerCallback_t)();

class Debugger : public sgl::NonCopyable {
protected:
  sgl::View* view;
  sgl::OpenGL::Shader shader_wireframe;
  sgl::OpenGL::Font fonts[4]; /* regular, bold, italic, bold italic */
  struct {
    int x, y, w, h;
  } textbox;
  /*
  Variance shadow mapping.
  */
  struct {
    Vec3 position;
    Vec3 look_at;
    double near_dist;
    double far_dist;
    double range;
    int resolution;
  } light;
  sgl::OpenGL::FrameBuffer fbuf_render, fbuf_blur, fbuf_VSM;
  sgl::OpenGL::Texture tex_color, tex_normal;
  sgl::OpenGL::Texture tex_VSM, tex_VSM_blur;
  sgl::OpenGL::Shader shader_VSM_main, shader_VSM_gen, shader_VSM_blur;
  /*
  For each draw call, we cache all necessary rendering data in a std::map,
  using the object's physical address as the key and the cached data as the
  value. This allows us to quickly retrieve the precomputed data when the
  same object is rendered again, significantly reducing the time spent
  rebuilding GPU-required resources.
  */
  std::map<uint64_t, std::unique_ptr<DebuggerCachedData>> cached_geometries;
  /*
  We also create some preset geometries, they will be used for visualization.
  [0]: a unit sphere cage with r = 1.0
  [1]: a unit segment v(1,0,0)
  */
  std::vector<std::unique_ptr<DebuggerCachedData>> preset_geometries;

  /*
  Physics simulation.
  */
  std::vector<RigidBody*> bodies;           /* not owned */
  std::vector<BaseConstraint*> constraints; /* not owned */
  std::vector<RigidBody*> watched_bodies;   /* not owned */

  sgl::Timer frame_timer;
  struct {
    int cur_id;
    int remaining;
    double dt;
    double T;
    int substeps;
    std::string temp_log;
  } frame_info;
  bool _enable_on_pause_callback;
  DebuggerCallback_t on_start; /* will be called at frame 0 */
  DebuggerCallback_t on_pause; /* will be called when remaining frames == 0 */

  /*
  visualization
  */
  bool show_mesh_;
  bool show_cage_;
  bool cast_shadow_;
  bool show_velocities_;

protected:

  DebuggerCachedData& _cache_and_fetch_geometry(const Convex& convex);
  DebuggerCachedData& _cache_and_fetch_geometry(const RigidBody& body);
  void _delete_cached_geometry(const uint64_t id);

  void _draw(const Convex& object, const Vec3& position, const Quat& rotation, const Vec3& scale, const Vec3& color);
  void _draw(const Convex& object, const Mat4x4& model_matrix, const Vec3& color);
  void _draw(const RigidBody& entity);
  void _draw_segment(const Vec3& p1, const Vec3& p2, const Vec3& color);
  void _draw_wireframe_indicators(const RigidBody& body);
  void _log_status();

  bool _save_RigidBody_states(const RigidBody& body, const std::string& file) const;
  bool _load_RigidBody_states(RigidBody& body, const std::string& file) const;

  /*
  Scene visualization
  */
  void _render_without_shadow();
  void _render_with_shadow();
  void _render_with_shadow_RenderScene(sgl::OpenGL::Shader& shader);
  Mat4x4 _render_with_shadow_LightPass();
  void _render_with_shadow_BlurPass();
  void _render_with_shadow_MainPass(Mat4x4& light_transform);

public:

  /*
  Should be set at initialization.
  */

  bool initialize();
  void set_view(View* view);
  void add_rigid_body(RigidBody* body);
  void add_watch(RigidBody* body);
  void set_textbox(int x, int y, int w, int h);
  void set_light(const Vec3& position, const Vec3& look_at, double near_dist, double far_dist, double range);

  /*
  Debugging functionalities.
  */
  void pause_after_n_frames(int n);
  void pause(); /* pause immediately and call on_pause callback if available */
  void resume();
  int get_current_frame_id() const;
  bool save_all_bodies_states(const std::string& out_zip_file) const;
  bool load_all_bodies_states(const std::string& in_zip_file) const;
  void set_callbacks(
    DebuggerCallback_t on_start_fn,
    DebuggerCallback_t on_pause_fn
  );
  void cast_shadow(bool state = true);
  void show_cage(bool state = true);
  void show_mesh(bool state = true);
  void show_velocities(bool state = true);
  bool can_cast_shadow() const;
  bool can_show_cage() const;
  bool can_show_mesh() const;
  bool can_show_velocities() const;

  /*
  Place this in main loop to drive all debugger logics.
  */
  void run(double dt, int substeps);
  void run_realtime(int substeps);

  /*
  Others.
  */
  void delete_cached_geometry();

public:
  Debugger();
  virtual ~Debugger();
};


};
};

#endif
