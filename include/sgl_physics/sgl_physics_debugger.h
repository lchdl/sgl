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

typedef void(*DebuggerBreakpointCallback_t)();

class Debugger {
protected:
  sgl::EyeParams* eye;
  sgl::OpenGL::Shader shaders[2];
  sgl::OpenGL::Font fonts[4]; /* regular, bold, italic, bold italic */
  /*
  For each draw call, we cache all necessary rendering data in a std::map,
  using the object's physical address as the key and the cached data as the
  value. This allows us to quickly retrieve the precomputed data when the
  same object is rendered again, significantly reducing the time spent
  rebuilding GPU-required resources.
  */
  std::map<uint64_t, std::unique_ptr<DebuggerCachedData>> cached_geometry;

  std::vector<RigidBody*> bodies;
  std::vector<BaseConstraint*> constraints;
  std::vector<RigidBody*> watched_bodies;

  sgl::Timer frame_timer;
  struct {
    int cur_id;
    int remaining;
    double dt;
    int substeps;
  } frame_info;
  bool enable_breakpoint_callback;
  DebuggerBreakpointCallback_t breakpoint_callback;
  struct {
    int x, y, w, h;
  } textbox;

protected:

  DebuggerCachedData& _cache_and_fetch_geometry(const convex& object);
  DebuggerCachedData& _cache_and_fetch_geometry(const RigidBody& entity);
  void _delete_cached_geometry(const uint64_t id);

  void _draw(const convex& object, const Vec3& position, const Quat& rotation, const Vec3& scale, const Vec3& color);
  void _draw(const convex& object, const Mat4x4& model_matrix, const Vec3& color);
  void _draw(const RigidBody& entity);
  void _log_status();

  bool _save_RigidBody_states(const RigidBody& body, const std::string& file) const;
  bool _load_RigidBody_states(RigidBody& body, const std::string& file) const;

public:
  /*
  Should be set at initialization.
  */
  bool initialize();
  void set_eye_params(EyeParams* eye);
  void add_rigid_body(RigidBody* body);
  void add_watch(RigidBody* entity);
  void set_textbox(int x, int y, int w, int h);

  /*
  Debugging functionalities.
  */
  void step_n_frames(int n);
  int get_current_frame_id() const;
  bool save_all_bodies_states(const std::string& out_zip_file) const;
  bool load_all_bodies_states(const std::string& in_zip_file) const;
  void set_breakpoint_callback(DebuggerBreakpointCallback_t fn);

  /*
  Place this in main loop to drive all debugger logics.
  */
  void run_fixed_dt(double dt, int substeps);
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
