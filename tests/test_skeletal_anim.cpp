#include <stdio.h>

#include "sgl.h"

using namespace sgl;

int w = 320, h = 240;
int num_threads = -1;
DrawMode draw_mode = DrawMode::triangle_draw_mode;
bool keystate[SDL_NUM_SCANCODES];
int show_texture = 1;
bool backface_culling = true;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

Model boblamp_model;
BaseAnimator animator;
Pipeline pipeline;
Texture color_texture, depth_texture, normal_texture;

void
init_env(int argc, char* argv[]) {
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

void 
destroy_env() {
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
}

void
process_key(SDL_KeyboardEvent *key) {
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
    if (animator.eye.perspective.enabled) {
      animator.eye.perspective.enabled = false;
      animator.eye.orthographic.enabled = true;
      printf("Now enables orthographic projection.\n");
    }
    else {
      animator.eye.perspective.enabled = true;
      animator.eye.orthographic.enabled = false;
      printf("Now enables perspective projection.\n");
    }
  }
  if (keycode == SDLK_RETURN && is_press) {
    if (draw_mode == DrawMode::triangle_draw_mode) {
      draw_mode = DrawMode::wireframe_draw_mode;
      printf("Now uses DrawMode::wireframe_draw_mode.\n");
    }
    else if (draw_mode == DrawMode::wireframe_draw_mode) {
      draw_mode = DrawMode::triangle_draw_mode;
      printf("Now uses DrawMode::triangle_draw_mode.\n");
    }
    pipeline.set_draw_mode(draw_mode);
  }
  if (keycode == SDLK_1 && is_press) {
    show_texture = 1;
    printf("Now display color components.\n");
  }
  else if (keycode == SDLK_2 && is_press) {
    show_texture = 2;
    printf("Now display depth components.\n");
  }
  else if (keycode == SDLK_3 && is_press) {
    show_texture = 3;
    printf("Now display normal maps.\n");
  }
  if (keycode == SDLK_b && is_press) {
    if (backface_culling == false) {
      backface_culling = true;
      printf("Backface culling: ON\n");
    }
    else {
      backface_culling = false;
      printf("Backface culling: OFF\n");
    }
    pipeline.enable_backface_culling(backface_culling);
  }
}

void
init_render() {
  /* Step 1: Setup resources. */
  color_texture.create(w, h,
    PixelFormat::pixel_format_BGRA8888,
    SamplingMode::texture_sampling_point,
    TextureUsage::color_components);
  depth_texture.create(w, h,
    PixelFormat::pixel_format_float64,
    SamplingMode::texture_sampling_point,
    TextureUsage::depth_buffer);
  normal_texture.create(w, h,
    PixelFormat::pixel_format_BGRA8888,
    SamplingMode::texture_sampling_point,
    TextureUsage::color_components);
  boblamp_model.load("models/boblamp.zip");
  boblamp_model.dump();

  /* Step 2: Setup render pass. */
  animator.out_texs.color = &color_texture;
  animator.out_texs.depth = &depth_texture;
  animator.out_texs.normal = &normal_texture;
  animator.eye.position = Vec3(0, 6, 10);
  animator.eye.look_at = Vec3(0, 3.5, 0);
  animator.eye.up_dir = Vec3(0, 1, 0);
  /* perspective */
  animator.eye.perspective.enabled = true;
  animator.eye.perspective.near = 1.0;
  animator.eye.perspective.far = 50.0;
  animator.eye.perspective.field_of_view = degrees_to_radians(60.0);
  /* orthographic */
  animator.eye.orthographic.enabled = false;
  animator.eye.orthographic.near = 1.0;
  animator.eye.orthographic.far = 50.0;
  animator.eye.orthographic.width = 12.0;
  animator.eye.orthographic.height = 9.0;
  /* setup model to be rendered */
  animator.model = &boblamp_model;
  animator.pipeline = &pipeline;
  pipeline.set_draw_mode(DrawMode::triangle_draw_mode);
  
  if (num_threads > 0) {
    pipeline.set_num_threads(num_threads);
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
  animator.play_time = fmod(T, 6.0); /* 6 seconds per loop */
  animator.anim_name = ""; /* play the animation "" */
  animator.eye.position = Vec3(radius * sin(T / 3), 6, radius * cos(T / 3));
  animator.eye.look_at = Vec3(0, 3.5, 0);
  animator.run();
  return animator.last_draw_time;
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

  /* Start main loop */
  SDL_Event e;
  Timer global_timer, frame_timer;
  double T_global = 0.0, T_frame = 0.0;
  int frameid = 0;

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* timing */
    frameid++;
    frame_timer.tick();
    T_global += global_timer.tick();
    
    /* render the whole frame */
    double draw_time = render_frame(T_global);
        
    /* logging */
    double frame_time = frame_timer.tick();
    T_frame += draw_time;
    if (show_texture == 1) {
      sgl::SDL2::sgl_texture_to_SDL2_surface(animator.out_texs.color, pWindowSurface);
    }
    else if (show_texture == 2) {
      sgl::SDL2::sgl_texture_to_SDL2_surface(animator.out_texs.depth, pWindowSurface);
    }
    else if (show_texture == 3) {
      sgl::SDL2::sgl_texture_to_SDL2_surface(animator.out_texs.normal, pWindowSurface);
    }
    SDL_UpdateWindowSurface(pWindow);
    char buf[64];
    sprintf(buf, "%.2lfms, T=%.2lfs", T_frame / frameid * 1000.0, T_global);
    std::string title = std::string("SGL | ") + buf + " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}
