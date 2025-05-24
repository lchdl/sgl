#include <stdio.h>

#ifdef ENABLE_OPENGL

#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
const int w = 640, h = 480;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

struct {
  sgl::View view;
  sgl::Model wedge, box, teapot, cone, sphere;
} gl;

struct {
  const double dt = 0.016;
  const int substeps = 4;
  Physics::RigidBody wedges[2], boxes[2], teapots[2], cones[16], spheres[4];
  Physics::Debugger debugger;
} phys;

void on_pause_callback()
{
  //printf("A breakpoint was hit at frame %d.\n", phys.debugger.get_current_frame_id());
  /*phys.debugger.save_all_bodies_states(
    sgl::abspath(
      std::string("states/frame_") + std::to_string(phys.debugger.get_current_frame_id()) + ".zip"
    )
  );*/
}

void on_start_callback()
{
  phys.debugger.save_all_bodies_states("states/test_physics_initial.zip");
}

void reset_view() {
  gl.view.eye.position = Vec3(0, 9, 12);
  gl.view.eye.look_at = Vec3(0, 4, 0);
  gl.view.eye.up_dir = Vec3(0, 1, 0);
  gl.view.eye.perspective.enabled = true;
  gl.view.eye.perspective.near = 0.1;
  gl.view.eye.perspective.far = 30.0;
  gl.view.eye.perspective.field_of_view = degrees_to_radians(60.0);
}
void do_camera_movement(double dt)
{
  const double translate_speed = 10.0;
  const double rotate_speed = 80.0;

  double translate_amount = dt * translate_speed;
  double rotate_amount = dt * rotate_speed;

  auto get_code = [](const std::string& key_name) -> int {
    return (int)SDL_GetScancodeFromName(key_name.c_str());
  };

  if (keystate[get_code("Space")])      gl.view.move_up(translate_amount);
  if (keystate[get_code("Left Shift")]) gl.view.move_down(translate_amount);
  if (keystate[get_code("W")])          gl.view.move_forward(translate_amount);
  if (keystate[get_code("A")])          gl.view.move_left(translate_amount);
  if (keystate[get_code("S")])          gl.view.move_backward(translate_amount);
  if (keystate[get_code("D")])          gl.view.move_right(translate_amount);
  if (keystate[get_code("Up")])         gl.view.rotate_up(rotate_amount);
  if (keystate[get_code("Down")])       gl.view.rotate_down(rotate_amount);
  if (keystate[get_code("Left")])       gl.view.rotate_left(rotate_amount);
  if (keystate[get_code("Right")])      gl.view.rotate_right(rotate_amount);
}

std::string dtos(double v, int precision) {
  std::stringstream stream;
  stream << std::fixed << std::setprecision(precision) << v;
  return stream.str();
}

void init_env(int argc, char* argv[]) {
  SDL_SetMainReady();

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);
  /* Create window */
  pWindow = SDL_CreateWindow("SGL Physics Simulation", 
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 
    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (pWindow == NULL)
    exit(1);
  if (!sgl::OpenGL::initialize_OpenGL(pWindow, 4, 3, false, true))
    exit(1);
  SDL_ShowWindow(pWindow);
  pWindowSurface = SDL_GetWindowSurface(pWindow);
  SDL_UpdateWindowSurface(pWindow);

  for (uint32_t i_key = 0; i_key < SDL_NUM_SCANCODES; i_key++)
    keystate[i_key] = false;

  /* set current working directory */
  set_cwd(gd(argv[0]));
}

void destroy_env() {
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
}

void process_key(SDL_KeyboardEvent *key) {
  bool is_press = (key->type == SDL_KEYDOWN);
  /* scancode is based on QWERTY layout,
   * while keycode generated from the same key position
   * can be different from different keyboard layouts. */
  uint32_t scancode = key->keysym.scancode;
  uint32_t keycode = key->keysym.sym;
  std::string keyname = SDL_GetKeyName(keycode);
  /* record key state */
  keystate[scancode] = is_press ? true : false;

  /* custom key handling */
  if (keyname == "F9" && is_press) {
    phys.debugger.resume();
  }
  else if (keyname == "F10" && is_press) {
    phys.debugger.pause_after_n_frames(1);
  }
  else if (keyname == "F11" && is_press) {
    phys.debugger.load_all_bodies_states("states/test_physics_initial.zip");
  }
  else if (keyname == "F1" && is_press) {
    if (phys.debugger.can_cast_shadow())
      phys.debugger.cast_shadow(false);
    else
      phys.debugger.cast_shadow(true);
  }
  else if (keyname == "F2" && is_press) {
    if (phys.debugger.can_show_cage())
      phys.debugger.show_cage(false);
    else
      phys.debugger.show_cage(true);
  }
  else if (keyname == "F3" && is_press) {
    if (phys.debugger.can_show_velocities())
      phys.debugger.show_velocities(false);
    else
      phys.debugger.show_velocities(true);
  }
  else if (keyname == "F4" && is_press) {
    if (phys.debugger.can_show_mesh())
      phys.debugger.show_mesh(false);
    else
      phys.debugger.show_mesh(true);
  }
  else if (keyname == "F5" && is_press) {
    reset_view();
  }

}

void init_render_and_physics() {

  printf("\n");
  printf("F1: Toggle cast shadow.\n");
  printf("F2: Toggle show body cage.\n");
  printf("F3: Toggle show velocities.\n");
  printf("F4: Toggle show mesh.\n");
  printf("\n");
  printf("F9: Continue.\n");
  printf("F10: Step over.\n");
  printf("F11: Reset simulation.\n");
  printf("\n");
  printf("F5: Reset camera.\n");
  printf("W/A/S/D: Move camera.\n");
  printf("Up/Down/Left/Right: Rotate camera.\n");
  printf("Space/LShift: Raise/Lower the camera.\n");
  printf("\n");

  reset_view();

  sgl::Physics::Convex hull;

  gl.teapot.load_zip("assets/common/models/teapot_lowpoly.zip", "teapot_lowpoly.obj");
  hull = sgl::Physics::build_convex_3D(SimpleOBJLoader::load_v("assets/common/models/teapot_lowpoly_convhull.obj"));
  phys.teapots[0].buildConvex(1, hull, 1.0, hull.center_of_mass(), 1.0, &gl.teapot);
  phys.teapots[0].setPosition(Vec3(-4.5, 6, 0));
  phys.teapots[0].setName("teapot0");
  phys.teapots[1].buildConvex(2, hull, 1.0, hull.center_of_mass(), 1.0, &gl.teapot);
  phys.teapots[1].setPosition(Vec3(+4.5, 6, 0));
  phys.teapots[1].setRotation(Quat::rot_y(PI));
  phys.teapots[1].setName("teapot1");

  gl.box.load_zip("assets/common/models/box.zip", "box.obj");
  hull = sgl::Physics::build_convex_3D(gl.box);
  phys.boxes[0].buildConvex(3, hull, 1.0, hull.center_of_mass(), 1.0, &gl.box);
  phys.boxes[0].setPosition(Vec3(-4.5, 4.5, 0));
  phys.boxes[0].setName("box0");
  phys.boxes[1].buildConvex(4, hull, 1.0, hull.center_of_mass(), 1.0, &gl.box);
  phys.boxes[1].setPosition(Vec3(+4.5, 4.5, 0));
  phys.boxes[1].setRotation(Quat::rot_y(PI));
  phys.boxes[1].setName("box1");

  gl.wedge.load_zip("assets/common/models/wedge.zip", "wedge.obj");
  hull = sgl::Physics::build_convex_3D(SimpleOBJLoader::load_v("assets/common/models/wedge_convhull.obj"));
  phys.wedges[0].buildConvex(5, hull, 1.0, hull.center_of_mass(), 1.0, &gl.wedge);
  phys.wedges[0].setPosition(Vec3(-1.5, 0, 0));
  phys.wedges[0].setStatic();
  phys.wedges[0].setName("wedge0");
  phys.wedges[1].buildConvex(5, hull, 1.0, hull.center_of_mass(), 1.0, &gl.wedge);
  phys.wedges[1].setPosition(Vec3(+1.5, 0, 0));
  phys.wedges[1].setRotation(Quat::rot_y(PI));
  phys.wedges[1].setStatic();
  phys.wedges[1].setName("wedge1");

  gl.cone.load_zip("assets/common/models/traffic_cone.zip", "traffic_cone.obj");
  hull = sgl::Physics::build_convex_3D(gl.cone);
  for (int i = 0; i < 8; i++) {
    phys.cones[i].buildConvex(10 + i, hull, 0.5, hull.center_of_mass(), 1.0, &gl.cone);
    phys.cones[i].setPosition(Vec3(-3.5, 10 + i * 1.1, 2));
    phys.cones[i].setName(std::string("cone") + std::to_string(i));
    phys.debugger.add_rigid_body(&phys.cones[i]);
  }
  for (int i = 8; i < 16; i++) {
    phys.cones[i].buildConvex(20 + i, hull, 0.5, hull.center_of_mass(), 1.0, &gl.cone);
    phys.cones[i].setPosition(Vec3(+3.5, 10 + (i - 8) * 1.1, -2));
    phys.cones[i].setName(std::string("cone") + std::to_string(i));
    phys.debugger.add_rigid_body(&phys.cones[i]);
  }

  gl.sphere.load_zip("assets/common/models/unit_sphere.zip", "unit_sphere.obj");
  phys.spheres[0].buildSphere(30, 1.0, 0.5, 0.3, &gl.sphere, Vec3(0.0, 0.0, 0.0));
  phys.spheres[0].setPosition(Vec3(-2, 10, 3));
  phys.spheres[0].setName("sphere0");
  phys.spheres[1].buildSphere(31, 1.0, 0.5, 0.3, &gl.sphere, Vec3(0.0, 0.0, 0.0));
  phys.spheres[1].setPosition(Vec3(-2, 11, 3));
  phys.spheres[1].setName("sphere1");
  phys.spheres[2].buildSphere(32, 1.0, 0.5, 0.3, &gl.sphere, Vec3(0.0, 0.0, 0.0));
  phys.spheres[2].setPosition(Vec3(2, 10, 3));
  phys.spheres[2].setName("sphere2");
  phys.spheres[3].buildSphere(33, 1.0, 0.5, 0.3, &gl.sphere, Vec3(0.0, 0.0, 0.0));
  phys.spheres[3].setPosition(Vec3(2, 11, 3));
  phys.spheres[3].setName("sphere3");

  phys.debugger.set_callbacks(on_start_callback, on_pause_callback);
  phys.debugger.add_rigid_body(&phys.wedges[0]);
  phys.debugger.add_rigid_body(&phys.wedges[1]);
  phys.debugger.add_rigid_body(&phys.boxes[0]);
  phys.debugger.add_rigid_body(&phys.boxes[1]);
  phys.debugger.add_rigid_body(&phys.teapots[0]);
  phys.debugger.add_rigid_body(&phys.teapots[1]);
  for (int i = 0; i < 4; i++)
    phys.debugger.add_rigid_body(&phys.spheres[i]);

  phys.debugger.initialize();
  phys.debugger.set_textbox(1, 1, 300, 300);
  phys.debugger.set_view(&gl.view);
  phys.debugger.add_watch(&phys.boxes[0]);
  phys.debugger.add_watch(&phys.teapots[0]);
    
  phys.debugger.set_light(Vec3(0.0, 10.0, 0.0), Vec3(0.0, 0.0, 0.0), 0.001, 10.0, 8.0);
  phys.debugger.cast_shadow(true);
}

double render_frame(double T)
{
  sgl::Timer timer;
  timer.tick();

  /* run debugger */
  //phys.debugger.run(phys.dt, phys.substeps);
  phys.debugger.run_realtime(phys.substeps);

  return timer.tick();
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render_and_physics();

  /* Start main loop */
  SDL_Event e;
  sgl::Timer timer;  

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* render & timing */
    double dt = timer.tick();
    do_camera_movement(dt);

    T_global += dt;
    T_frame += render_frame(T_global);
    frameid++;

    /* Swap buffers */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(pWindow);
    std::string title = std::string("SGL Physics Simulation | ") + 
      dtos(T_frame / frameid * 1000.0, 3) + "ms | FPS=" + 
      std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());

  }

  destroy_env();
  return 0;
}


#else
int main() {
  printf("OpenGL hardware acceleration (\"ENABLE_OPENGL\" flag) is disabled.\n");
}
#endif