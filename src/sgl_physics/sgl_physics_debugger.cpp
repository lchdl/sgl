#include "sgl_physics/sgl_physics_debugger.h"
#include <sstream>
#include <limits>

#ifdef ENABLE_OPENGL

namespace sgl {
namespace Physics {

DebuggerCachedData & Debugger::_cache_and_fetch_geometry(const Convex & convex) {
  uint64_t id = reinterpret_cast<uint64_t>(&convex);
  if (this->cached_geometries.find(id) == this->cached_geometries.end()) {
    this->cached_geometries.emplace(id, std::make_unique<DebuggerCachedData>());
    /* setup convex hull data */
    std::vector<DebuggerCachedData::Vertex> vertices;
    DebuggerCachedData::Vertex v;
    v.texcoord[0] = 0.0f;
    v.texcoord[1] = 0.0f;
    for (int i = 0; i < convex.num_points(); i++) {
      v.position[0] = float(convex.get_point(i).x);
      v.position[1] = float(convex.get_point(i).y);
      v.position[2] = float(convex.get_point(i).z);
      v.normal[0] = float(convex.get_normal(i).x);
      v.normal[1] = float(convex.get_normal(i).y);
      v.normal[2] = float(convex.get_normal(i).z);
      vertices.push_back(v);
    }
    std::vector<IVec3> faces;
    for (int i = 0; i < convex.num_faces(); i++) {
      faces.push_back(convex.get_triangle_indices(i));
    }
    cached_geometries[id]->conv_hull_vbuf.create_and_fill(convex.num_points(), sizeof(DebuggerCachedData::Vertex),
      vertices.data(), GL_STATIC_DRAW, 3 * convex.num_faces(), sizeof(int), faces.data(), GL_STATIC_DRAW);
  }
  return *(cached_geometries[id]);
}

DebuggerCachedData & Debugger::_cache_and_fetch_geometry(const RigidBody & body)
{
  uint64_t id = reinterpret_cast<uint64_t>(&body);
  if (this->cached_geometries.find(id) == this->cached_geometries.end()) {
    this->cached_geometries.emplace(id, std::make_unique<DebuggerCachedData>());

    /* setup convex hull data */
    std::vector<DebuggerCachedData::Vertex> vertices;
    DebuggerCachedData::Vertex v;
    v.texcoord[0] = 0.0f;
    v.texcoord[1] = 0.0f;
    const Convex& convex_hull = body.collider.convexMeshCollider.convexHull;
    for (int i = 0; i < convex_hull.num_points(); i++) {
      v.position[0] = float(convex_hull.get_point(i).x);
      v.position[1] = float(convex_hull.get_point(i).y);
      v.position[2] = float(convex_hull.get_point(i).z);
      v.normal[0] = float(convex_hull.get_normal(i).x);
      v.normal[1] = float(convex_hull.get_normal(i).y);
      v.normal[2] = float(convex_hull.get_normal(i).z);
      vertices.push_back(v);
    }
    std::vector<IVec3> faces;
    for (int i = 0; i < convex_hull.num_faces(); i++) {
      faces.push_back(convex_hull.get_triangle_indices(i));
    }
    cached_geometries[id]->conv_hull_vbuf.create_and_fill(convex_hull.num_points(), sizeof(DebuggerCachedData::Vertex),
      vertices.data(), GL_STATIC_DRAW, 3 * convex_hull.num_faces(), sizeof(int), faces.data(), GL_STATIC_DRAW);

    /* setup model data */
    cached_geometries[id]->model = body.model;
    if (body.model != NULL)
      cached_geometries[id]->renderer.set_model(body.model, 1);
    cached_geometries[id]->renderer.set_view_params(this->view);
  }
  return *(cached_geometries[id]);
}

void Debugger::_delete_cached_geometry(const uint64_t id) {
  if (this->cached_geometries.find(id) != this->cached_geometries.end())
    this->cached_geometries.erase(id);
}

void Debugger::delete_cached_geometry() {
  this->cached_geometries.clear();
}

bool Debugger::initialize()
{
  if (sgl::OpenGL::is_OpenGL_initialized() == false)
    return false;

  /*
  Setup fonts and shader
  */
  typedef sgl::OpenGL::Shader::FragDataLoc FragDataLoc;
  FragDataLoc fs_outs[] = {
    {"FragColor", 0},
    {"FragNormal", 1},
  };
  this->shader_wireframe.create(
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/wireframe.vert"),
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/wireframe.frag"),
    sizeof(fs_outs) / sizeof(FragDataLoc), fs_outs
  );
  this->fonts[0].load("assets/common/fonts/Arial/11pt_Regular.fnt");
  this->fonts[1].load("assets/common/fonts/Arial/11pt_Bold.fnt");
  this->fonts[2].load("assets/common/fonts/Arial/11pt_Italic.fnt");
  this->fonts[3].load("assets/common/fonts/Arial/11pt_BoldItalic.fnt");

  /*
  Setup framebuffer
  */
  SDL_Window* window = sgl::OpenGL::get_SDL_window();
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  this->tex_color.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  this->tex_color.to_device(DeviceType_GPU);
  this->tex_normal.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  this->tex_normal.to_device(DeviceType_GPU);
  this->fbuf_render.setup_attachment(&this->tex_color, 0);
  this->fbuf_render.setup_attachment(&this->tex_normal, 1);
  this->fbuf_render.make();

  /* variance shadow mapping */
  this->tex_VSM.create(this->light.resolution, this->light.resolution, PixelFormat_OpenGL_RG32F, TextureSampling_Bilinear, TextureUsage_ColorComponents);
  this->tex_VSM.to_device(DeviceType_GPU);
  this->fbuf_VSM.setup_attachment(&this->tex_VSM, 0);
  this->fbuf_VSM.make();
  sgl::OpenGL::Shader::FragDataLoc vsm_gen_outs[] = {
    {"FragVSM", 0},
  };
  this->shader_VSM_gen.create(
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_gen.vert"),
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_gen.frag"),
    sizeof(vsm_gen_outs) / sizeof(sgl::OpenGL::Shader::FragDataLoc), vsm_gen_outs
  );

  /* variance shadow mapping: blur */
  this->tex_VSM_blur.create(this->light.resolution, this->light.resolution, PixelFormat_OpenGL_RG32F, TextureSampling_Bilinear, TextureUsage_ColorComponents);
  this->tex_VSM_blur.set_wrap_mode(sgl::OpenGL::TextureWrapMode_ClampToBorder);
  this->tex_VSM_blur.set_border_color(Vec4(1.0, 1.0, 1.0, 1.0));
  this->tex_VSM_blur.to_device(DeviceType_GPU);
  this->fbuf_blur.setup_attachment(&this->tex_VSM_blur, 0);
  this->fbuf_blur.make();
  sgl::OpenGL::Shader::FragDataLoc vsm_blur_outs[] = {
    {"FragColor", 0},
  };
  this->shader_VSM_blur.create(
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_blur.vert"),
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_blur.frag"),
    sizeof(vsm_blur_outs) / sizeof(sgl::OpenGL::Shader::FragDataLoc), vsm_blur_outs
  );

  /* variance shadow mapping: main */
  sgl::OpenGL::Shader::FragDataLoc vsm_main_outs[] = {
  {"FragColor", 0},
  };
  this->shader_VSM_main.create(
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_main.vert"),
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/vsm_main.frag"),
    sizeof(vsm_main_outs) / sizeof(sgl::OpenGL::Shader::FragDataLoc), vsm_main_outs
  );

  /*
  Create preset geometries
  */

  /* a unit sphere cage */
  std::vector<DebuggerCachedData::Vertex> vertices;
  auto build_unit_sphere_cage = []()-> std::vector<DebuggerCachedData::Vertex> {
    std::vector<DebuggerCachedData::Vertex> vertices;
    DebuggerCachedData::Vertex v[2];
    v[0].texcoord[0] = 0.0f; v[0].texcoord[1] = 0.0f;
    v[1].texcoord[0] = 0.0f; v[1].texcoord[1] = 0.0f;
    const int resolution = 16; /* number of segments */
    Vec3 vprev, vcur;
    /* xy plane */
    vprev.z = vcur.z = 0.0f;
    for (int i = 0; i <= resolution; i++) {
      float theta = float((2 * sgl::PI) / resolution * i);
      vcur.x = cos(theta); vcur.y = sin(theta);
      v[1].position[0] = float(vcur.x); v[1].position[1] = float(vcur.y); v[1].position[2] = float(vcur.z);
      if (i > 0) {
        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
      }
      v[0] = v[1];
    }
    /* xz plane */
    vprev.y = vcur.y = 0.0f;
    for (int i = 0; i <= resolution; i++) {
      float theta = float((2 * sgl::PI) / resolution * i);
      vcur.x = cos(theta); vcur.z = sin(theta);
      v[1].position[0] = float(vcur.x); v[1].position[1] = float(vcur.y); v[1].position[2] = float(vcur.z);
      if (i > 0) {
        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
      }
      v[0] = v[1];
    }
    /* yz plane */
    vprev.x = vcur.x = 0.0f;
    for (int i = 0; i <= resolution; i++) {
      float theta = float((2 * sgl::PI) / resolution * i);
      vcur.y = cos(theta); vcur.z = sin(theta);
      v[1].position[0] = float(vcur.x); v[1].position[1] = float(vcur.y); v[1].position[2] = float(vcur.z);
      if (i > 0) {
        vertices.push_back(v[0]);
        vertices.push_back(v[1]);
      }
      v[0] = v[1];
    }
    return vertices;
  };
  this->preset_geometries.push_back(std::make_unique<DebuggerCachedData>());
  vertices = build_unit_sphere_cage();
  this->preset_geometries[0]->conv_hull_vbuf.create_and_fill(
    len(vertices), sizeof(DebuggerCachedData::Vertex),
    vertices.data(), GL_STATIC_DRAW, 0, 0, NULL, GL_STATIC_DRAW);

  /* unit segment v(1,0,0) */
  auto build_unit_segment = []() -> std::vector<DebuggerCachedData::Vertex> {
    std::vector<DebuggerCachedData::Vertex> v(2);
    v[0].position[0] = 0.0f; v[0].position[1] = 0.0f; v[0].position[2] = 0.0f;
    v[1].position[0] = 1.0f; v[1].position[1] = 0.0f; v[1].position[2] = 0.0f;
    return v;
  };
  this->preset_geometries.push_back(std::make_unique<DebuggerCachedData>());
  vertices = build_unit_segment();
  this->preset_geometries[1]->conv_hull_vbuf.create_and_fill(
    len(vertices), sizeof(DebuggerCachedData::Vertex),
    vertices.data(), GL_STATIC_DRAW, 0, 0, NULL, GL_STATIC_DRAW);

  return true;
}

void Debugger::run(double dt, int substeps)
{
  /*
  Debugger logics
  */
  if (frame_info.cur_id == 0) {
    if (on_start)
      on_start();
  }
  if (frame_info.remaining > 0) {
    if (dt <= 0.0) {
      printf("Invalid time step: dt must be greater than 0.\n");
      return;
    }
    double h = dt / substeps;
    if (h < 0.0001 || h > 0.005) {
      printf("Warning: time step `h` is outside the recommended range "
        "[0.0001, 0.0050]. Simulation may become unstable or produce "
        "inaccurate results.\n");
    }
    XPBDSolver::update(bodies, constraints, dt, substeps, Vec3(0.0, -9.8, 0.0));
    frame_info.dt = dt;
    frame_info.T += dt;
    frame_info.substeps = substeps;
    frame_info.cur_id++;
    if (frame_info.remaining != std::numeric_limits<int>::max())
      frame_info.remaining--;
    /*
    Body states is updated, break point callback 
    can be activated and waiting to be called. 
    */
    _enable_on_pause_callback = true;
  }
  else {
    if (_enable_on_pause_callback && on_pause) {
      on_pause();
    }
    _enable_on_pause_callback = false;
  }

  /*
  Render & logging
  */
  if (this->cast_shadow_ == false) {
    this->_render_without_shadow();
  }
  else {
    this->_render_with_shadow();
  }
  _log_status();
  frame_info.temp_log = "";
}

void Debugger::pause() {
  frame_info.remaining = 0;
  _enable_on_pause_callback = true;
}

void Debugger::resume() {
  frame_info.remaining = std::numeric_limits<int>::max();
  _enable_on_pause_callback = true;
}

void Debugger::run_realtime(int subframes)
{  
  double dt_since_last_call = frame_timer.tick();  
  double dt_actual = clamp(0.0001 * subframes, dt_since_last_call, 0.0050 * subframes); /* ensure h = dt / subframes stays in safe range [0.0001, 0.0050] */
  double simulation_speed = dt_actual / dt_since_last_call;

  if (simulation_speed != 1.0) {
    auto format_double = [](double d, int precision) -> std::string {
      std::stringstream x;
      x << std::fixed << std::setprecision(precision) << d;
      return x.str();
    };
    frame_info.temp_log = std::string("Simulation speed: ") + format_double(simulation_speed, 2) + std::string("x.");
  }
  else {
    frame_info.temp_log = std::string("Simulation runs in realtime.");
  }
  run(dt_actual, subframes);
}

void Debugger::_draw(const Convex& object, const Vec3& position, const Quat& rotation, const Vec3& scale, const Vec3& color)
{
  if (this->view == NULL)
    return;

  Mat4x4 model_matrix = Mat4x4::translate(position.x, position.y, position.z) * Mat4x4(quat_to_mat3x3(rotation)) * Mat4x4::scale(scale.x, scale.y, scale.z);
  this->_draw(object, model_matrix, color);
}

void Debugger::_draw(const Convex & object, const Mat4x4 & model_matrix, const Vec3& color)
{
  if (this->view == NULL)
    return;

  DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(object);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  Mat4x4 view = this->view->get_view_matrix();
  Mat4x4 proj = this->view->get_projection_matrix(rsize.x, rsize.y);

  this->shader_wireframe.use();
  this->shader_wireframe.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);
  this->shader_wireframe.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
  this->shader_wireframe.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);
  this->shader_wireframe.set_uniform_3f("color", float(color.r), float(color.g), float(color.b));
  this->shader_wireframe.set_uniform_1f("u_dz", -0.001f); /* avoid z-fighting between mesh and wireframe */
  /* draw convex in wireframe mode, note that OpenGL ES does not support this */
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  cached_data.conv_hull_vbuf.draw_elements(GL_TRIANGLES, object.num_faces() * 3, GL_UNSIGNED_INT, NULL);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Debugger::_draw(const RigidBody& body)
{
  if (this->view == NULL)
    return;

  DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(body);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();

  /* * * * * * * * * */
  /* draw mesh model */
  /* * * * * * * * * */

  if (this->show_mesh_ && cached_data.model != NULL) {
    /*
    Note: The model mesh's center of mass may not be located at the origin.
    The offset is stored in the model_CoM_offset variable and is accounted
    for here. We use right multiplication here to ensure that the center of
    mass offset transform is applied before other transformations.
    */
    Mat4x4 model_matrix = 
      Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
      Mat4x4(quat_to_mat3x3(body.pose.q)) * 
      Mat4x4::scale(body.scale, body.scale, body.scale) * 
      Mat4x4::translate(-body.modelOffset.x, -body.modelOffset.y, -body.modelOffset.z);
    /* view and projection matrix will be caluclated automatically by renderer */
    cached_data.renderer.set_model_transform(model_matrix);
    cached_data.renderer.draw();
  }

  /* * * * * * * * * */
  /* draw wireframe  */
  /* * * * * * * * * */
  _draw_wireframe_indicators(body);

}

void Debugger::_draw_segment(const Vec3 & p1, const Vec3 & p2, const Vec3& color)
{
  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();

  Mat4x4 view = this->view->get_view_matrix();
  Mat4x4 proj = this->view->get_projection_matrix(rsize.x, rsize.y);
  this->shader_wireframe.use();
  this->shader_wireframe.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
  this->shader_wireframe.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);
  this->shader_wireframe.set_uniform_3f("color", float(color.r), float(color.g), float(color.b));
  this->shader_wireframe.set_uniform_1f("u_dz", 0.0f); 

  Mat4x4 transform = Mat4x4::translate(p1.x, p1.y, p1.z) * compute_transform(Vec3(1, 0, 0), p2 - p1);
  this->shader_wireframe.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &transform);

  glEnable(GL_CULL_FACE);
  preset_geometries[1]->conv_hull_vbuf.draw_arrays(GL_LINES, 0, preset_geometries[1]->conv_hull_vbuf.get_num_vertices());
  glDisable(GL_CULL_FACE);
}

void Debugger::_draw_wireframe_indicators(const RigidBody& body)
{
  DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(body);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();

  if (this->show_cage_) {
    Mat4x4 view = this->view->get_view_matrix();
    Mat4x4 proj = this->view->get_projection_matrix(rsize.x, rsize.y);
    this->shader_wireframe.use();
    this->shader_wireframe.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
    this->shader_wireframe.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);
    this->shader_wireframe.set_uniform_3f("color", 1.0f, 0.0f, 0.0f);
    this->shader_wireframe.set_uniform_1f("u_dz", -0.0005f); /* avoid z-fighting between mesh and wireframe */

    if (body.collider.colliderType == ColliderType_ConvexMesh) {

      /* Ignore scaling (set as 1.0) since we already considered scaling before */
      Mat4x4 model_matrix =
        Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
        Mat4x4(quat_to_mat3x3(body.pose.q));
      this->shader_wireframe.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);

      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
      glDisable(GL_CULL_FACE);
      cached_data.conv_hull_vbuf.draw_elements(GL_TRIANGLES, body.collider.convexMeshCollider.convexHull.num_faces() * 3, GL_UNSIGNED_INT, NULL);
      glEnable(GL_CULL_FACE);
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    else if (body.collider.colliderType == ColliderType_Sphere) {
      Mat4x4 model_matrix =
        Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
        Mat4x4(quat_to_mat3x3(body.pose.q)) *
        Mat4x4::scale(body.scale * 1.02, body.scale * 1.02, body.scale * 1.02); /* 1% larger */
      this->shader_wireframe.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);

      glDisable(GL_CULL_FACE);
      preset_geometries[0]->conv_hull_vbuf.draw_arrays(GL_LINES, 0, preset_geometries[0]->conv_hull_vbuf.get_num_vertices());
      glEnable(GL_CULL_FACE);
    }
  }

  if (this->show_velocities_) {
    /* draw auxiliary information (velocity) */
    if (!body.vel.is_zero()) {
      double seglen = (1.0 + length(body.vel) * 0.1) * body.collider.radius;
      _draw_segment(body.pose.p, body.pose.p + seglen * normalize(body.vel), Vec3(0.0, 0.0, 1.0));
    }
    if (!body.omega.is_zero()) {
      double seglen = (1.0 + length(body.vel) * 0.1) * body.collider.radius;
      _draw_segment(body.pose.p, body.pose.p + seglen * normalize(body.omega), Vec3(0.0, 1.0, 1.0));
    }
  }

}

void Debugger::_log_status()
{
  const int line_height = 10;
  const Vec4 BLACK  = Vec4(0.0, 0.0, 0.0, 1.0);
  const Vec4 WHITE  = Vec4(1.0, 1.0, 1.0, 1.0);
  const Vec4 RED    = Vec4(1.0, 0.0, 0.0, 1.0);
  const Vec4 GREEN  = Vec4(0.0, 1.0, 0.0, 1.0);
  const Vec4 BLUE   = Vec4(0.0, 0.0, 1.0, 1.0);
  const Vec4 YELLOW = Vec4(1.0, 1.0, 0.0, 1.0);
  const int precision = 4; /* decimal points */
  const int REGULAR = 0, BOLD = 1, ITALIC = 2, BOLD_ITALIC = 3;

  int x_cur = textbox.x, y_cur = textbox.y;
  std::wstring line;

  auto format_Vec3 = [](const Vec3& v, int precision) -> std::wstring {
    std::wstringstream x, y, z;
    x << std::fixed << std::showpos << std::setprecision(precision) << v.x;
    y << std::fixed << std::showpos << std::setprecision(precision) << v.y;
    z << std::fixed << std::showpos << std::setprecision(precision) << v.z;
    std::wstring s = L"(" + x.str() + L"X, " + y.str() + L"Y, " + z.str() + L"Z)";
    return s;
  };
  auto format_Quat = [](const Quat& q, int precision) -> std::wstring {
    std::wstringstream x, y, z, s;
    x << std::fixed << std::showpos << std::setprecision(precision) << q.x;
    y << std::fixed << std::showpos << std::setprecision(precision) << q.y;
    z << std::fixed << std::showpos << std::setprecision(precision) << q.z;
    s << std::fixed << std::showpos << std::setprecision(precision) << q.s;
    std::wstring r = L"(" + s.str() + L"S, " + x.str() + L"X, " + y.str() + L"Y, " + z.str() + L"Z)";
    return r;
  };
  auto format_double = [](double d, int precision) -> std::wstring {
    std::wstringstream x;
    x << std::fixed << std::setprecision(precision) << d;
    return x.str();
  };

  auto print = [&](const Vec4& color, const int STYLE) -> void {
    this->fonts[STYLE].draw_text(line, x_cur, y_cur, textbox.w, textbox.h - x_cur, color);
    y_cur += line_height;
  };

  line = L"Physics Debugger Status";
  print(WHITE, BOLD);
  if (frame_info.remaining > 0) {
    line = L"Frame: " + std::to_wstring(frame_info.cur_id) + L".";
    if (frame_info.remaining != std::numeric_limits<int>::max())
      line += L" Remaining: " + std::to_wstring(frame_info.remaining) + L".";
    print(GREEN, BOLD);
  }
  else {
    line = L"Frame: " + std::to_wstring(frame_info.cur_id) + L". Simulation paused.";
    print(RED, BOLD);
  }

  line = L"T = " + format_double(this->frame_info.T, 3) + L" s, "
    L"dt = " + format_double(frame_info.dt * 1000.0, 2) + L" ms, " + 
    L"steps = " + std::to_wstring(frame_info.substeps) + L", " +
    L"h = " + format_double(frame_info.dt / frame_info.substeps * 1000.0, 2) + L" ms.";
  print(YELLOW, BOLD);

  if (frame_info.temp_log != "") {
    line = sgl::utf8string_to_wstring(frame_info.temp_log);
    print(YELLOW, BOLD);
  }

  for (int i = 0; i < len(watched_bodies); i++) {
    RigidBody* watched_body = watched_bodies[i];
    line = L"Entity name: \"" + sgl::utf8string_to_wstring(watched_body->name) + L"\".";
    print(WHITE, BOLD);
    line = L"Cur.Pos = " + format_Vec3(watched_body->pose.p, precision);
    print(WHITE, REGULAR);
    line = L"Cur.Rot = " + format_Quat(watched_body->pose.q, precision);
    print(WHITE, REGULAR);
    line = L"Cur.LinVel = " + format_Vec3(watched_body->vel, precision);
    print(WHITE, REGULAR);
    line = L"Cur.AngVel = " + format_Vec3(watched_body->omega, precision);
    print(WHITE, REGULAR);
  }
}

void Debugger::pause_after_n_frames(int n)
{
  this->frame_info.remaining = n;
}

int Debugger::get_current_frame_id() const
{
  return this->frame_info.cur_id;
}

void Debugger::set_callbacks(
  DebuggerCallback_t on_start_fn, 
  DebuggerCallback_t on_stop_fn)
{
  this->on_start = on_start_fn;
  this->on_pause = on_stop_fn;
}

bool Debugger::_save_RigidBody_states(const RigidBody& body, const std::string& file) const
{
  FILE* fp = fopen(file.c_str(), "wb");
  if (fp == NULL) return false;
  auto writeInt = [&fp](int i) -> void {
    fwrite(&i, sizeof(int), 1, fp);
  };
  auto writeString = [&fp, writeInt](const std::string& s) -> void {
    size_t l = strlen(s.c_str());
    writeInt(int(l));
    fwrite(s.c_str(), 1, l, fp);
  };
  auto writeDouble = [&fp](double d) -> void {
    fwrite(&d, sizeof(double), 1, fp);
  };
  auto writeVec3 = [&fp](const Vec3& v) -> void {
    fwrite(&v.i[0], sizeof(Vec3), 1, fp);
  };
  auto writeQuat = [&fp](const Quat& q) -> void {
    fwrite(&q.i[0], sizeof(Quat), 1, fp);
  };
  auto writePose = [&fp, writeVec3, writeQuat](const Pose& pose) -> void {
    writeVec3(pose.p);
    writeQuat(pose.q);
  };
  auto writeMat3x3 = [&fp](const Mat3x3& m) -> void {
    fwrite(&m.i[0], sizeof(Mat3x3), 1, fp);
  };
  auto writeBool = [&fp, writeInt](bool b) -> void {
    if (b) writeInt(1);
    else   writeInt(0);
  };
  /* write file header */
  std::string magic = "gVMVS0*8up17DpNzf0JUT@SLqKEmJUrm";

  writeString(magic);
  writeInt(body.cid);
  writePose(body.pose);
  writeVec3(body.vel);
  writeVec3(body.omega);
  writeDouble(body.invMass);
  writeMat3x3(body.invLocalInertia);
  writeVec3(body.force);
  writeVec3(body.torque);
  writeDouble(body.gravity);
  writeDouble(body.staticFriction);
  writeDouble(body.dynamicFriction);
  writeDouble(body.restitution);
  writePose(body.prevPose);
  writeVec3(body.prevVel);
  writeVec3(body.prevOmega);
  writeBool(body.canCollide);
  writeBool(body.canSleep);
  writeBool(body.isDynamic);
  writeBool(body.isSleeping);
  writeBool(body.hasStableContact);
  writeString(body.name);

  fclose(fp);
  return true;
}

bool Debugger::_load_RigidBody_states(RigidBody& body, const std::string& file) const
{
  FILE* fp = fopen(file.c_str(), "rb");
  if (fp == NULL) return false;
  auto readInt = [&fp]() -> int {
    int i;
    fread(&i, sizeof(int), 1, fp);
    return i;
  };
  auto readString = [&fp, readInt]() -> std::string {
    int l = readInt();
    int bufsize = sizeof(char)*(l + 1);
    char* buf = (char*)malloc(bufsize);
    memset(buf, 0, bufsize);
    fread(buf, sizeof(char), l, fp);
    return std::string(buf);
  };
  auto readDouble = [&fp]() -> double {
    double d;
    fread(&d, sizeof(double), 1, fp);
    return d;
  };
  auto readVec3 = [&fp]() -> Vec3 {
    Vec3 v;
    fread(&v.i[0], sizeof(Vec3), 1, fp);
    return v;
  };
  auto readQuat = [&fp]() -> Quat {
    Quat q;
    fread(&q.i[0], sizeof(Quat), 1, fp);
    return q;
  };
  auto readPose = [&fp, readVec3, readQuat]() -> Pose {
    Pose pose;
    pose.p = readVec3();
    pose.q = readQuat();
    return pose;
  };
  auto readMat3x3 = [&fp]() -> Mat3x3 {
    Mat3x3 m;
    fread(&m.i[0], sizeof(Mat3x3), 1, fp);
    return m;
  };
  auto readBool = [&fp, readInt]() -> bool {
    int i = readInt();
    return i > 0 ? true : false;
  };

  /* read file header */
  std::string magic = "gVMVS0*8up17DpNzf0JUT@SLqKEmJUrm";
  if (readString() != magic) {
    fclose(fp);
    return false;
  }
  body.cid = readInt();
  body.pose = readPose();
  body.vel = readVec3();
  body.omega = readVec3();
  body.invMass = readDouble();
  body.invLocalInertia = readMat3x3();
  body.force = readVec3();
  body.torque = readVec3();
  body.gravity = readDouble();
  body.staticFriction = readDouble();
  body.dynamicFriction = readDouble();
  body.restitution = readDouble();
  body.prevPose = readPose();
  body.prevVel = readVec3();
  body.prevOmega = readVec3();
  body.canCollide = readBool();
  body.canSleep = readBool();
  body.isDynamic = readBool();
  body.isSleeping = readBool();
  body.hasStableContact = readBool();
  body.name = readString();

  fclose(fp);
  return true;
}

void Debugger::_render_without_shadow()
{
  this->fbuf_render.bind();
  {

    glClearColor(0.36f, 0.36f, 0.36f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    for (int i = 0; i < len(bodies); i++)
      _draw(*bodies[i]);

  }
  this->fbuf_render.unbind();
  this->fbuf_render.blit_attachment_to_main_framebuffer(0, 0, 0, this->fbuf_render.get_width(), this->fbuf_render.get_height());
}

void Debugger::_render_with_shadow()
{
  {
    Mat4x4 light_xfm;
    light_xfm = _render_with_shadow_LightPass();
    _render_with_shadow_BlurPass();
    _render_with_shadow_MainPass(light_xfm);
  }
  this->fbuf_render.blit_attachment_to_main_framebuffer(0, 0, 0, this->fbuf_render.get_width(), this->fbuf_render.get_height());

  /* Show variance shadow map */
  const double show_scale = 0.25;
  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  const int w = rsize.x, h = rsize.y;
  sgl::OpenGL::blit_texture(
    &this->tex_VSM,
    w, h,
    0, 0, this->light.resolution, this->light.resolution,
    w, 0,
    Vec2(show_scale, show_scale),
    0, Vec3(1, 1, 1), SpriteOriginMode_TopRight);
  this->fonts[0].draw_text(L"Variance Shadow Map",
    rsize.x - int(show_scale * this->light.resolution) + 1, 1, Vec4(0.0, 0.0, 0.0, 1.0));

}

void Debugger::_render_with_shadow_RenderScene(sgl::OpenGL::Shader & shader)
{
  for (int i = 0; i < len(this->bodies); i++) {

    RigidBody& body = *bodies[i];

    if (this->show_mesh_ && body.model) {

      Mat4x4 model_matrix =
        Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
        Mat4x4(quat_to_mat3x3(body.pose.q)) *
        Mat4x4::scale(body.scale, body.scale, body.scale) *
        Mat4x4::translate(-body.modelOffset.x, -body.modelOffset.y, -body.modelOffset.z);
      shader.set_uniform_matrix_4fv("Model", 1, GL_TRUE, &model_matrix);

      /* we get all the model related resources (textures, meshes, and materials) and draw it */
      DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(body);
      sgl::OpenGL::AnimatedModelRenderer& renderer = cached_data.renderer;
      const std::map<void*, sgl::OpenGL::Texture*>& texmap = renderer.get_texmap();
      const sgl::Model* model = renderer.get_model();
      const std::vector<sgl::Mesh>& mesh_data = model->get_meshes();
      const std::vector<Material>& materials = model->get_materials();
      const std::vector<sgl::OpenGL::AnimatedModelRenderer::VertexBuffer_t*>&
        vbufs = renderer.get_vertex_buffers();

      for (int i_mesh = 0; i_mesh < len(mesh_data); i_mesh++) {
        /* load diffuse texture into shader */
        const int32_t mat_id = mesh_data[i_mesh].mat_id;
        void* tex_cpu_dptr = materials[mat_id].diffuse_texture.get_pixel_data();
        if (texmap.find(tex_cpu_dptr) != texmap.end()) {
          if (shader.get_uniform_location("tex1") != -1) {
            std::map<void*, sgl::OpenGL::Texture*>::const_iterator 
              it = texmap.find(tex_cpu_dptr);
            const sgl::OpenGL::Texture* diffuse_texture = it->second;
            shader.set_texture_sampler_2D("tex1", *diffuse_texture, 0);
          }
        }
        vbufs[i_mesh]->draw_elements(GL_TRIANGLES, vbufs[i_mesh]->get_num_indices(), GL_UNSIGNED_INT, NULL);
      }

    }
  }
}

Mat4x4 Debugger::_render_with_shadow_LightPass()
{
  Mat4x4 light_xfm;

  this->fbuf_VSM.bind();
  {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); /* depth buffer should set to farthest 1.0f */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    const int w = fbuf_VSM.get_width(), h = fbuf_VSM.get_height();

    /* setup light */

    auto get_up_dir = [](Vec3 pos, Vec3 look)-> Vec3 {
      Vec3 up = Vec3(0, 1, 0);
      Vec3 t = cross(look - pos, up);
      if (length(t) < 0.0001)
        return Vec3(0, 0, 1);
      else
        return up;
    };
    Mat4x4 light_view = sgl::get_view_matrix(this->light.position, this->light.look_at, get_up_dir(this->light.position, this->light.look_at));
    Mat4x4 light_proj = sgl::get_orthographic_matrix(this->light.near_dist, this->light.far_dist, 
      -this->light.range, +this->light.range, +this->light.range, -this->light.range);
    light_xfm = light_proj * light_view;

    /* setup shader */

    this->shader_VSM_gen.use();
    this->shader_VSM_gen.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_xfm);

    /* draw each body */
    this->_render_with_shadow_RenderScene(this->shader_VSM_gen);

  }
  this->fbuf_VSM.unbind();

  return light_xfm;
}

void Debugger::_render_with_shadow_BlurPass()
{
  this->fbuf_blur.bind();
  {
    const int w = this->fbuf_blur.get_width(), h = this->fbuf_blur.get_height();
    /* here note the custom shader instance we passed to blit_texture() */
    sgl::OpenGL::blit_texture(&this->tex_VSM, w, h, 0, 0, w, h, 0, 0, Vec2(1, 1), 0, Vec3(1, 1, 1), SpriteOriginMode_TopLeft, &this->shader_VSM_blur);
  }
  this->fbuf_blur.unbind();
}

void Debugger::_render_with_shadow_MainPass(Mat4x4& light_transform)
{
  this->fbuf_render.bind();
  {
    glClearColor(0.36f, 0.36f, 0.36f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    const int w = fbuf_render.get_width(), h = fbuf_render.get_height();

    Mat4x4 view_matrix = this->view->get_view_matrix();
    Mat4x4 proj_matrix = this->view->get_projection_matrix(w, h);

    this->shader_VSM_main.use();
    this->shader_VSM_main.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_transform);
    this->shader_VSM_main.set_uniform_matrix_4fv("View", 1, GL_TRUE, &view_matrix);
    this->shader_VSM_main.set_uniform_matrix_4fv("Projection", 1, GL_TRUE, &proj_matrix);
    this->shader_VSM_main.set_texture_sampler_2D("tex_VSM", this->tex_VSM_blur, 1);

    this->_render_with_shadow_RenderScene(this->shader_VSM_main);

    for (int i = 0; i < len(bodies); i++)
      this->_draw_wireframe_indicators(*bodies[i]);

  }
  this->fbuf_render.unbind();
}

bool Debugger::save_all_bodies_states(const std::string& out_zip_file) const
{
  bool success = true;

  std::string tdir = mktdir(gd(out_zip_file));

  int n_bodies = len(bodies);
  std::vector<std::string> state_files(n_bodies);
  const char** files_sz = (const char**)malloc(sizeof(char*) * n_bodies);

  for (int i = 0; i < n_bodies; i++) {
    std::string file = join(tdir, std::to_string(i) + "_BodyStates.xpbd");
    state_files[i] = file;
    files_sz[i] = state_files[i].c_str();
    if (!_save_RigidBody_states(*bodies[i], file)) {
      success = false;
      break;
    }
  }

  if (success) {
    /* zip all files */
    if (zip_create(out_zip_file.c_str(), files_sz, n_bodies) != 0)
      success = false;
  }

  rm(tdir);
  free(files_sz);
  return success;
}

bool Debugger::load_all_bodies_states(const std::string& in_zip_file) const
{
  bool success = true;

  std::string tdir = mktdir(gd(in_zip_file));

  int zipret = zip_extract(in_zip_file.c_str(), tdir.c_str(), NULL, NULL);
  if (zipret < 0) {
    success = false;
  }

  for (int i = 0; i < len(bodies); i++) {
    std::string file = join(tdir, std::to_string(i) + "_BodyStates.xpbd");
    if (!_load_RigidBody_states(*bodies[i], file)) {
      success = false;
      break;
    }
  }

  rm(tdir);
  return success;
}

void Debugger::add_watch(RigidBody * body)
{
  this->watched_bodies.push_back(body);
}

void Debugger::set_textbox(int x, int y, int w, int h)
{
  textbox.x = x;
  textbox.y = y;
  textbox.w = w;
  textbox.h = h;
}

void Debugger::cast_shadow(bool state)
{
  this->cast_shadow_ = state;
}

void Debugger::show_cage(bool state) {
  this->show_cage_ = state;
}

void Debugger::show_mesh(bool state) { 
  this->show_mesh_ = state;
}

void Debugger::show_velocities(bool state) {
  this->show_velocities_ = state;
}

bool Debugger::can_cast_shadow() const
{
  return this->cast_shadow_;
}

bool Debugger::can_show_cage() const {
  return this->show_cage_;
}

bool Debugger::can_show_mesh() const {
  return this->show_mesh_;
}

bool Debugger::can_show_velocities() const {
  return this->show_velocities_;
}

void Debugger::set_light(const Vec3 & position, const Vec3 & look_at, double near_dist, double far_dist, double range)
{
  this->light.position = position;
  this->light.look_at = look_at;
  this->light.near_dist = near_dist;
  this->light.far_dist = far_dist;
  this->light.range = range;
}

void Debugger::set_view(View* view)
{
  this->view = view;
}

void Debugger::add_rigid_body(RigidBody * body)
{
  this->bodies.push_back(body);
}

Debugger::Debugger() 
{
  this->view = NULL; 
  this->frame_info.cur_id = 0;
  this->frame_info.dt = 0.0;
  this->frame_info.T = 0.0;
  this->frame_info.remaining = std::numeric_limits<int>::max();
  this->frame_info.substeps = 1;
  this->_enable_on_pause_callback = true;
  this->on_start = NULL;
  this->on_pause = NULL;
  this->cast_shadow_ = true;
  this->show_cage_ = true;
  this->show_velocities_ = true;
  this->show_mesh_ = true;
  this->light.position = Vec3(0.0, 10.0, 0.0);
  this->light.near_dist = 0.1;
  this->light.far_dist = 20.0;
  this->light.range = 10.0;
  this->light.resolution = 512;
}

Debugger::~Debugger()
{
  this->delete_cached_geometry();
}

DebuggerCachedData::DebuggerCachedData()
{
  model = NULL;
}

DebuggerCachedData::~DebuggerCachedData()
{
  conv_hull_vbuf.destroy();
}


};
};

#endif
