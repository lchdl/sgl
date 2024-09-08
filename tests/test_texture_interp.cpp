#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

int w = 512, h = 512;
struct {
  Texture checker;
  Texture target;
} textures;
SpriteRenderer renderer;

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
  pWindow = SDL_CreateWindow("SGL | Texture Interpolation Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
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
    if (textures.checker.get_sampling_mode() == TextureSampling_Nearest) {
      textures.checker.set_sampling_mode(TextureSampling_Bilinear);
    }
    else if (textures.checker.get_sampling_mode() == TextureSampling_Bilinear) {
      textures.checker.set_sampling_mode(TextureSampling_Nearest);
    }
  }
}

void init_render() {
  /* setup resources */
  textures.checker = sgl::load_texture("assets/tests/textures/checker_5x5.png", PixelFormat_BGRA8888, TextureSampling_Nearest);
  textures.target = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);

  /* initialize render pipeline */
  renderer.set_render_target(&textures.target);
  renderer.clear_render_target(Vec4(0.5, 0.5, 0.5, 1.0));
  renderer.set_num_threads(2);

  printf("\n");
  printf("Press SPACE to switch between point/linear interpolation modes.\n");
  printf("Press ESC to quit this demo.\n");
  printf("\n");
}

void render_frame() {
  renderer.draw(&textures.checker, Vec2(w / 2, h / 2), Vec2(64, 64), 0, Vec3(1, 1, 1));
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

  /* Start main loop */
  SDL_Event e;
  Timer global_timer;

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* render */
    render_frame();
    sgl::SDL2::sgl_texture_to_SDL2_surface(&textures.target, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
  }

  destroy_env();
  return 0;
}
