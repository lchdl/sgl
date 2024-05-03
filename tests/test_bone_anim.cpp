#include <stdio.h>

#include "sgl_SDL2.h"
#include "sgl_pipeline.h"
#include "sgl_model.h"
#include "sgl_utils.h"


#include "assimp/quaternion.h"

using namespace sgl;

int w = 800, h = 600;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
bool keystate[SDL_NUM_SCANCODES];

ModelPass render_pass;
Model model;
Pipeline pipeline;
Texture colortex, depthtex;

void
init_render_pass() {
  /* Step 1: Setup resources. */
  colortex.create(w, h,
                  TextureFormat::texture_format_RGBA8,
                  TextureSampling::texture_sampling_point);
  depthtex.create(w, h,
                  TextureFormat::texture_format_float64,
                  TextureSampling::texture_sampling_point);

  /* Step 2: Setup render config. */
  render_pass.eye.position = Vec3(0, 100, 100);
  render_pass.eye.look_at = Vec3(0, 0, 0);
  render_pass.eye.up_dir = Vec3(0, 1, 0);
  render_pass.eye.perspective.enabled = true;
  render_pass.eye.perspective.near = 1;
  render_pass.eye.perspective.far = 500.0;
  render_pass.eye.perspective.field_of_view = PI / 3.0;
  render_pass.color_texture = &colortex;
  render_pass.depth_texture = &depthtex;
  render_pass.FS = model_FS;
  render_pass.VS = model_VS;

  /* Step 3: Initialize model. */ 
  model.load("models/boblampclean.zip");
  render_pass.model = &model;

	//pipeline.set_num_threads(1);
	//pipeline.disable_backface_culling();

}

void record_key(SDL_KeyboardEvent *key) { 
  bool is_press = (key->type == SDL_KEYUP);
  /* scancode is based on QWERTY layout,
   * while keycode generated from the same key position 
   * can be different from different keyboard layouts. */
  uint32_t scancode = key->keysym.scancode;
  uint32_t keycode = key->keysym.sym;
  std::string keyname = SDL_GetKeyName(keycode);
  /* record key state */
  keystate[scancode] = is_press ? true : false;
}
//
//void handle_key() {
//  if (keystate[SDL_SCANCODE_W])
//}

int
main(int argc, char* argv[]) {

  SDL_SetMainReady();

  set_cwd(gd(argv[0]));

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);

  for (uint32_t i_key = 0; i_key < SDL_NUM_SCANCODES; i_key++)
    keystate[i_key] = false;

  /* Create window */
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (pWindow == NULL)
    exit(1);
  
  SDL_ShowWindow(pWindow);

  /* Get window surface */
  pWindowSurface = SDL_GetWindowSurface(pWindow);

  init_render_pass();

  SDL_UpdateWindowSurface(pWindow);

  /* Start main loop */
  SDL_Event e;
  Timer timer, frame_timer;
  bool quit = false;
  int frameid = 0;
	double total_frame_time = 0.0, T = 0.0;
	char buf[64];
  while (!quit) {
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      quit = true;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      record_key(&e.key);

    frameid++;
    frame_timer.tick();
    T += timer.tick();
    
    /* render the whole frame */
    render_pass.time = fmod(T, 5.0);
    render_pass.anim_name = "";
    render_pass.run(pipeline);
    
    double frame_time = frame_timer.tick();
		total_frame_time += frame_time;
    sgl::SDL2::sgl_texture_to_SDL_surface(
        render_pass.color_texture, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
		sprintf(buf, "%.2lfms", total_frame_time / frameid * 1000.0);
    std::string title = std::string("SGL | ") + buf +
                        " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());

  }

  return 0;
}
