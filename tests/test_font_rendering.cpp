#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

int w = 640, h = 480;

sgl::Texture target;
sgl::Font MiniHerz_16pt;
sgl::Font Beatixel_16pt;
sgl::Font CuteBlockhead_16pt;
sgl::Font PixelFraktur_16pt;

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

  MiniHerz_16pt.load("assets/tests/fonts/MiniHerz/16pt_Regular.fnt");
  Beatixel_16pt.load("assets/tests/fonts/Beatixel/16pt_Regular.fnt");
  CuteBlockhead_16pt.load("assets/tests/fonts/CuteBlockhead/16pt_Regular.fnt");
  PixelFraktur_16pt.load("assets/tests/fonts/PixelFraktur/16pt_Regular.fnt");
}

double render_frame(double T) {
  sgl::Timer timer;
  timer.tick();

  std::string text_sample = sgl::read_file_as_string("assets/tests/texts/introduction_to_Deep_Learning.txt");
  sgl::replace_all(text_sample, "\n", " ");
  text_sample = sgl::truncate(text_sample, 6000);

  MiniHerz_16pt.draw(&target, text_sample.c_str(), 0, 0, w/2, h/2, Vec4(1, 1, 1, 1));
  Beatixel_16pt.draw(&target, text_sample.c_str(), w / 2, 0, w/2, h / 2, Vec4(1, 1, 1, 1));
  CuteBlockhead_16pt.draw(&target, text_sample.c_str(), 0, h / 2, w / 2, h / 2, Vec4(1, 1, 1, 1));
  PixelFraktur_16pt.draw(&target, text_sample.c_str(), w / 2, h / 2, w / 2, h / 2, Vec4(1, 1, 1, 1));

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
