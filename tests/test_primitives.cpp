#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

int w = 800, h = 600;

sgl::Texture target;
sgl::Texture chess, chess_mask;

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
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (pWindow == NULL)
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
  if (keycode == SDLK_SPACE && is_press) {
    /* do something here... */
  }
}

void init_render() {
  /* setup resources */
  target = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  target.clear(Vec4(0, 0, 0, 1));
  chess = sgl::load_texture("assets/tests/textures/chess.png", PixelFormat_BGRA8888);
  chess_mask = sgl::load_texture("assets/tests/textures/chess_mask.png", PixelFormat_UInt8);
}

double render_frame(double T) {
  sgl::Timer timer;
  timer.tick();

  sgl::draw_pixel(&target, 10, 20, Vec4(1, 1, 1, 1));
  sgl::draw_pixel(&target, 20, 10, Vec4(1, 0.5, 0.2, 1));
  sgl::draw_line(&target, 40, 60, 120, 80, Vec4(1, 1, 1, 1));
  sgl::draw_line(&target, 40, 60, 900, 80, Vec4(1, 1, 1, 1));
  sgl::draw_line(&target, 60, 40, 70, 200, Vec4(0, 1, 1, 1));
  sgl::draw_line(&target, 60, 40, 70, 860, Vec4(0, 1, 1, 1));
  sgl::draw_circle(&target, 300, 200, 150, Vec4(1, 0, 1, 1));
  sgl::draw_circle(&target, 300, 220, 26, Vec4(1, 0, 1, 1));
  sgl::draw_ellipse(&target, 200, 300, 100, 20, sgl::PI / 4, Vec4(1, 1, 0, 1), 128);
  sgl::draw_rectangle(&target, 100, 150, 50, 80, Vec4(0, 0.2, 1.0, 1.0));
  sgl::draw_rectangle(&target, 150, 180, 100, 60, 1.0, Vec4(1, 1, 0, 1));
  sgl::draw_bezier(&target, Vec2(50, 50), Vec2(100, 50), Vec2(50, 100), Vec2(100, 100), Vec4(1, 0, 0, 1), 64);

  sgl::blit_texture(&chess, &target, 0, 0, 96, 32, 100, 150, &chess_mask);

  return timer.tick();
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

  /* Start main loop */
  SDL_Event e;
  Timer global_timer;
  double T_global = 0.0, T_frame = 0.0;
  int frameid = 0;

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* render & timing */
    T_global += global_timer.tick();
    T_frame += render_frame(T_global);
    frameid++;

    sgl::SDL2::sgl_texture_to_SDL2_surface(&target, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 2) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}
