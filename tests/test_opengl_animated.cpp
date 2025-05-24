#include <stdio.h>

#ifdef ENABLE_OPENGL

#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
const int w = 240, h = 240;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

struct {
  sgl::Model boblamp;
  OpenGL::AnimatedModelRenderer animator;
  OpenGL::FrameBuffer framebuffer;
  OpenGL::Texture color_out0, color_out1;
  OpenGL::Font font;
  sgl::View view;
} gl;

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
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w * 2, h, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
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
}

void init_render() {
  gl.color_out0.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out0.to_device(DeviceType_GPU);
  gl.color_out1.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out1.to_device(DeviceType_GPU);
  gl.framebuffer.setup_attachment(&gl.color_out0, 0);
  gl.framebuffer.setup_attachment(&gl.color_out1, 1);
  gl.framebuffer.make();

  gl.font.load("assets/common/fonts/Arial/11pt_Regular.fnt");

  gl.boblamp.load_zip("assets/common/models/boblamp.zip", "model.md5mesh");
  gl.animator.set_model(&gl.boblamp, 2);
  gl.animator.set_model_transform(0, Vec3(+1.5, 0, +0.5), Vec3(1, 1, 1), Vec3(0, 1, 0), 0.0);
  gl.animator.set_model_transform(1, Vec3(-1.5, 0, -0.5), Vec3(1, 1, 1), Vec3(0, 1, 0), sgl::PI);
  gl.animator.set_view_params(&gl.view);

  gl.view.eye.position = Vec3(0, 6, 8);
  gl.view.eye.look_at = Vec3(0, 3, 0);
  gl.view.eye.up_dir = Vec3(0, 1, 0);
  /* perspective */
  gl.view.eye.perspective.enabled = true;
  gl.view.eye.perspective.near = 1.0;
  gl.view.eye.perspective.far = 50.0;
  gl.view.eye.perspective.field_of_view = degrees_to_radians(60.0);
  /* orthographic */
  gl.view.eye.orthographic.enabled = false;
  gl.view.eye.orthographic.near = 1.0;
  gl.view.eye.orthographic.far = 50.0;
  gl.view.eye.orthographic.width = 12.0;
  gl.view.eye.orthographic.height = 9.0;
}

void render_procedure(double T) {
  /* render to currently active framebuffer */

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  const int w = rsize.x, h = rsize.y;

  glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  const double radius = 8.0;
  gl.animator.play_animation(0, "", fmod(T, 6.0));
  gl.animator.play_animation(1, "", fmod(T + 3.0, 6.0));
  gl.view.eye.position = Vec3(radius * sin(T / 3), 6, radius * cos(T / 3));
  gl.view.eye.look_at = Vec3(0, 3.5, 0);
  gl.animator.draw();
}

double render_frame(double T) {

  sgl::Timer timer;
  timer.tick();

  gl.framebuffer.bind();
  {
    render_procedure(T);
  }
  gl.framebuffer.unbind();
  gl.framebuffer.blit_attachment_to_main_framebuffer(0, 0, 0, w, h);
  gl.framebuffer.blit_attachment_to_main_framebuffer(1, w, 0, w, h);

  gl.font.draw_text(L"Main View", 1, 0, Vec4(0, 0, 0, 1));
  gl.font.draw_text(L"Normals", w + 1, 0, Vec4(0, 0, 0, 1));

  return timer.tick();
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

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
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 3) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
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