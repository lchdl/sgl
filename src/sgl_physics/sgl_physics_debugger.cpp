#include "sgl_physics/sgl_physics_debugger.h"
#include <sstream>

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
    cached_geometries[id]->conv_hull_vbuf.create_and_fill(sizeof(DebuggerCachedData::Vertex) * convex.num_points(),
      vertices.data(), GL_STATIC_DRAW, sizeof(IVec3) * convex.num_faces(), faces.data(), GL_STATIC_DRAW);
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
    cached_geometries[id]->conv_hull_vbuf.create_and_fill(sizeof(DebuggerCachedData::Vertex) * convex_hull.num_points(),
      vertices.data(), GL_STATIC_DRAW, sizeof(IVec3) * convex_hull.num_faces(), faces.data(), GL_STATIC_DRAW);

    /* setup model data */
    cached_geometries[id]->model = body.model;
    if (body.model != NULL)
      cached_geometries[id]->renderer.set_model(body.model, 1);
    cached_geometries[id]->renderer.set_eye_params(this->eye);
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

  typedef sgl::OpenGL::Shader::FragDataLoc FragDataLoc;
  FragDataLoc fs_outs[] = {
    {"FragColor", 0},
    {"FragNormal", 1},
  };
  this->shader.create(
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/mesh.vert"),
    sgl::read_file_as_string("assets/common/shaders/PhysicsDebugger/mesh_wireframe.frag"),
    sizeof(fs_outs) / sizeof(FragDataLoc), fs_outs
  );
  this->fonts[0].load("assets/common/fonts/Arial/11pt_Regular.fnt");
  this->fonts[1].load("assets/common/fonts/Arial/11pt_Bold.fnt");
  this->fonts[2].load("assets/common/fonts/Arial/11pt_Italic.fnt");
  this->fonts[3].load("assets/common/fonts/Arial/11pt_BoldItalic.fnt");

  /*
  Create preset geometries
  */

  /* a unit sphere wireframe */
  this->preset_geometries.push_back(std::make_unique<DebuggerCachedData>());
  auto build_unit_sphere = []()-> std::vector<DebuggerCachedData::Vertex> {
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
  std::vector<DebuggerCachedData::Vertex> vertices = build_unit_sphere();
  this->preset_geometries[0]->n_vbuf_vertices = len(vertices);
  this->preset_geometries[0]->conv_hull_vbuf.create_and_fill(
    sizeof(DebuggerCachedData::Vertex) * len(vertices),
    vertices.data(), GL_STATIC_DRAW, 0, NULL, GL_STATIC_DRAW);

  return true;
}

void Debugger::run_fixed_dt(double dt, int substeps)
{
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
    frame_info.substeps = substeps;
    frame_info.cur_id++;
    frame_info.remaining--;
    /*
    Body states is updated, break point callback 
    can be activated and waiting to be called. 
    */
    enable_breakpoint_callback = true;
  }
  else {
    if (enable_breakpoint_callback && breakpoint_callback) {
      breakpoint_callback();
      enable_breakpoint_callback = false;
    }
  }

  /* 
  render & print log
  */

  for (int i = 0; i < len(bodies); i++)
    _draw(*bodies[i]);

  _log_status();
}

void Debugger::run_realtime(int subframes)
{
  double dt_since_last_run = frame_timer.tick();

  /* ensure h = dt / subframes stays in safe range [0.0001, 0.0050] */
  //dt_since_last_run = clamp(0.005, dt_since_last_run, 0.016);
  dt_since_last_run = clamp(0.0001 * subframes, dt_since_last_run, 0.0050 * subframes);

  run_fixed_dt(dt_since_last_run, subframes);

  /* TODO: if simulation runs not realtime, print warning (orange color) */
}


void Debugger::_draw(const Convex& object, const Vec3& position, const Quat& rotation, const Vec3& scale, const Vec3& color)
{
  if (this->eye == NULL)
    return;

  Mat4x4 model_matrix = Mat4x4::translate(position.x, position.y, position.z) * Mat4x4(quat_to_mat3x3(rotation)) * Mat4x4::scale(scale.x, scale.y, scale.z);
  this->_draw(object, model_matrix, color);
}

void Debugger::_draw(const Convex & object, const Mat4x4 & model_matrix, const Vec3& color)
{
  if (this->eye == NULL)
    return;

  DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(object);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  Mat4x4 view = this->eye->get_view_matrix();
  Mat4x4 proj = this->eye->get_projection_matrix(rsize.x, rsize.y);

  this->shader.use();
  this->shader.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);
  this->shader.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
  this->shader.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);
  this->shader.set_uniform_3f("color", float(color.r), float(color.g), float(color.b));
  this->shader.set_uniform_1f("u_dz", -0.001f); /* avoid z-fighting between mesh and wireframe */
  /* draw convex in wireframe mode, note that OpenGL ES does not support this */
  glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  cached_data.conv_hull_vbuf.draw_elements(GL_TRIANGLES, object.num_faces() * 3, GL_UNSIGNED_INT, NULL);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Debugger::_draw(const RigidBody& body)
{
  if (this->eye == NULL)
    return;

  DebuggerCachedData& cached_data = this->_cache_and_fetch_geometry(body);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();

  /* * * * * * * * * */
  /* draw mesh model */
  /* * * * * * * * * */

  if (cached_data.model != NULL) {
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
  
  Mat4x4 view = this->eye->get_view_matrix();
  Mat4x4 proj = this->eye->get_projection_matrix(rsize.x, rsize.y);
  this->shader.use();
  this->shader.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
  this->shader.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);
  this->shader.set_uniform_3f("color", 1.0f, 0.0f, 0.0f);
  this->shader.set_uniform_1f("u_dz", -0.0005f); /* avoid z-fighting between mesh and wireframe */

  if (body.collider.colliderType == ColliderType_ConvexMesh) {

    /* Ignore scaling (set as 1.0) since we already considered scaling before */
    Mat4x4 model_matrix =
      Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
      Mat4x4(quat_to_mat3x3(body.pose.q));
    this->shader.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    cached_data.conv_hull_vbuf.draw_elements(GL_TRIANGLES, body.collider.convexMeshCollider.convexHull.num_faces() * 3, GL_UNSIGNED_INT, NULL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
  }
  else if (body.collider.colliderType == ColliderType_Sphere) {

    Mat4x4 model_matrix =
      Mat4x4::translate(body.pose.p.x, body.pose.p.y, body.pose.p.z) *
      Mat4x4(quat_to_mat3x3(body.pose.q)) * 
      Mat4x4::scale(body.scale, body.scale, body.scale);
    this->shader.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &model_matrix);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    preset_geometries[0]->conv_hull_vbuf.draw_arrays(GL_LINES, 0, preset_geometries[0]->n_vbuf_vertices);
    glDisable(GL_CULL_FACE);
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

  line = L"Debugger Status Log";
  print(WHITE, BOLD);
  if (frame_info.remaining > 0) {
    line = L"Frame ID: " + std::to_wstring(frame_info.cur_id) + L". Remained: " + std::to_wstring(frame_info.remaining) + L".";
    print(GREEN, BOLD);
  }
  else {
    line = L"Frame ID: " + std::to_wstring(frame_info.cur_id) + L". Simulation stopped.";
    print(RED, BOLD);
  }

  line = L"dt = " + format_double(frame_info.dt * 1000.0, 2) + L" ms, " + 
    L"substeps = " + std::to_wstring(frame_info.substeps) + L", " +
    L"h = " + format_double(frame_info.dt / frame_info.substeps * 1000.0, 2) + L" ms.";
  print(YELLOW, BOLD);

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

void Debugger::step_n_frames(int n)
{
  this->frame_info.remaining += n;
}

int Debugger::get_current_frame_id() const
{
  return this->frame_info.cur_id;
}

void Debugger::set_breakpoint_callback(DebuggerBreakpointCallback_t fn)
{
  this->breakpoint_callback = fn;
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
  writeInt(body.id);
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
  body.id = readInt();
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

void Debugger::set_eye_params(EyeParams* eye)
{
  this->eye = eye;
}

void Debugger::add_rigid_body(RigidBody * body)
{
  this->bodies.push_back(body);
}

Debugger::Debugger() 
{
  this->eye = NULL; 
  this->frame_info.cur_id = 0;
  this->frame_info.dt = 0.0;
  this->frame_info.remaining = 0;
  this->frame_info.substeps = 1;
  this->enable_breakpoint_callback = true;
  this->breakpoint_callback = NULL;
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
