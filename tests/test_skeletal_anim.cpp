#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

int w = 320, h = 240;
int num_threads = 2;
int show_which_texture = 1;
struct {
  Texture color;
  Texture depth;
  Texture normal;
} frame_buffer;
AnimatedModelRenderer renderer;
EyeParams eye;

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
    if (eye.eye.perspective.enabled) {
      eye.eye.perspective.enabled = false;
      eye.eye.orthographic.enabled = true;
      printf("Now enables orthographic projection.\n");
    }
    else {
      eye.eye.perspective.enabled = true;
      eye.eye.orthographic.enabled = false;
      printf("Now enables perspective projection.\n");
    }
  }
  if (keycode == SDLK_RETURN && is_press) {
    PipelineDrawMode draw_mode = renderer.get_draw_mode();
    if (draw_mode == PipelineDrawMode_Triangle) {
      draw_mode = PipelineDrawMode_Wireframe;
      printf("Now uses DrawMode::wireframe_draw_mode.\n");
    }
    else if (draw_mode == PipelineDrawMode_Wireframe) {
      draw_mode = PipelineDrawMode_Triangle;
      printf("Now uses DrawMode::triangle_draw_mode.\n");
    }
    renderer.set_draw_mode(draw_mode);
  }
  if (keycode == SDLK_1 && is_press) {
    show_which_texture = 1;
    printf("Now display color components.\n");
  }
  else if (keycode == SDLK_2 && is_press) {
    show_which_texture = 2;
    printf("Now display depth components.\n");
  }
  else if (keycode == SDLK_3 && is_press) {
    show_which_texture = 3;
    printf("Now display normal maps.\n");
  }
  if (keycode == SDLK_b && is_press) {
    bool backface_culling = renderer.get_backface_culling_state();
    if (backface_culling == false) {
      backface_culling = true;
      printf("Backface culling: ON\n");
    }
    else {
      backface_culling = false;
      printf("Backface culling: OFF\n");
    }
    renderer.set_backface_culling_state(backface_culling);
  }
}

void init_render() {
  /* setup resources */
  frame_buffer.color = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  frame_buffer.depth = sgl::create_texture(w, h, PixelFormat_Float64, TextureSampling_Nearest, TextureUsage_DepthBuffer);
  frame_buffer.normal = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);

  /* setup render pass */
  renderer.bind_render_targets(&frame_buffer.color, &frame_buffer.depth, &frame_buffer.normal);
  renderer.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  renderer.set_eye_params(&eye);

  eye.eye.position = Vec3(0, 6, 10);
  eye.eye.look_at = Vec3(0, 3.5, 0);
  eye.eye.up_dir = Vec3(0, 1, 0);
  /* perspective */
  eye.eye.perspective.enabled = true;
  eye.eye.perspective.near = 1.0;
  eye.eye.perspective.far = 50.0;
  eye.eye.perspective.field_of_view = degrees_to_radians(60.0);
  /* orthographic */
  eye.eye.orthographic.enabled = false;
  eye.eye.orthographic.near = 1.0;
  eye.eye.orthographic.far = 50.0;
  eye.eye.orthographic.width = 12.0;
  eye.eye.orthographic.height = 9.0;

  /* setup model to be rendered */
  renderer.load_model_zip("assets/common/models/boblamp.zip", "model.md5mesh");
  renderer.set_draw_mode(PipelineDrawMode_Triangle);
  
  if (num_threads > 0) {
    renderer.set_pipeline_num_threads(num_threads);
  }
  printf("\n");
  printf("Press SPACE to switch between perspective/orthographic modes.\n");
  printf("Press ENTER to switch between normal/wireframe render modes.\n");
  printf("Press 1/2/3 to toggle color/depth/normal buffer display.\n");
  printf("Press B to toggle on/off backface culling.\n");
  printf("Press ESC to quit this demo.\n");
  printf("\n");
}

double render_frame(double T) {
  const double radius = 8.0;
  renderer.play_animation("", fmod(T, 6.0)); /* 6 seconds per loop */
  eye.eye.position = Vec3(radius * sin(T / 3), 6, radius * cos(T / 3));
  eye.eye.look_at = Vec3(0, 3.5, 0);
  renderer.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  renderer.draw();
  return renderer.query_last_draw_time();
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

    /* logging */
    if (show_which_texture == 1)
      sgl::SDL2::sgl_texture_to_SDL2_surface(&frame_buffer.color, pWindowSurface);
    else if (show_which_texture == 2)
      sgl::SDL2::sgl_texture_to_SDL2_surface(&frame_buffer.depth, pWindowSurface);
    else if (show_which_texture == 3)
      sgl::SDL2::sgl_texture_to_SDL2_surface(&frame_buffer.normal, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 2) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}
