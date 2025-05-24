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
  sgl::Model box1, sphere;
} gl;

struct {
  const double dt = 0.016;
  const int substeps = 4;
  Physics::RigidBody box1, sphere;
  Physics::Debugger debugger;
} phys;

void on_pause_callback()
{
  printf("A breakpoint was hit.\n");
  phys.debugger.save_all_bodies_states(
    sgl::abspath(
      std::string("states/frame_") + std::to_string(phys.debugger.get_current_frame_id()) + ".zip"
    )
  );
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
  if (keyname == "Space" && is_press) {
    phys.debugger.pause_after_n_frames(1);
  }
  
}

void init_render_and_physics() {

  gl.view.eye.position = Vec3(0, 9, 12);
  gl.view.eye.look_at = Vec3(0, 4, 0);
  gl.view.eye.up_dir = Vec3(0, 1, 0);
  gl.view.eye.perspective.enabled = true;
  gl.view.eye.perspective.near = 0.1;
  gl.view.eye.perspective.far = 30.0;
  gl.view.eye.perspective.field_of_view = degrees_to_radians(60.0);

  sgl::Physics::Convex hull;

  gl.box1.load_zip("assets/common/models/box1.zip", "box1.obj");
  hull = sgl::Physics::build_convex_3D(gl.box1);
  phys.box1.buildConvex(0, hull, 1.0, hull.center_of_mass(), 1.0, &gl.box1);
  phys.box1.setPosition(Vec3(0, 0, 0));
  phys.box1.setName("box1");
  phys.box1.setStatic();

  gl.sphere.load_zip("assets/common/models/unit_sphere.zip", "unit_sphere.obj");
  phys.sphere.buildSphere(1, 1.0, 0.5, 1.0, &gl.sphere, Vec3(0.0, 0.0, 0.0));
  phys.sphere.setPosition(Vec3(0, 10, 0));
  phys.sphere.setName("sphere");

  phys.debugger.initialize();
  phys.debugger.add_rigid_body(&phys.box1);
  phys.debugger.add_rigid_body(&phys.sphere);
  phys.debugger.set_callbacks(NULL, on_pause_callback);
  phys.debugger.set_textbox(1, 1, 300, 300);
  phys.debugger.set_view(&gl.view);
  phys.debugger.add_watch(&phys.sphere);
  phys.debugger.pause_after_n_frames(1000000);
}

double render_frame(double T)
{
  sgl::Timer timer;
  timer.tick();

  /* run debugger */
  Vec3 vel = phys.sphere.vel;
  //phys.sphere.setVel(Vec3(0.0, vel.y, 0.0));
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
    T_global = timer.elapsed();
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