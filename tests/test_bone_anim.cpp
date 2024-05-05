#include <stdio.h>

#include "sgl_pass.h"
#include "sgl_SDL2.h"

using namespace sgl;

int w = 800, h = 600;
bool keystate[SDL_NUM_SCANCODES];

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

ModelPass render_pass;
Model boblamp_model;
Pipeline pipeline;
Texture color_texture, depth_texture;

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
record_key(SDL_KeyboardEvent *key) {
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

void
init_render() {
	/* Step 1: Setup resources. */
	color_texture.create(w, h,
		TextureFormat::texture_format_RGBA8,
		TextureSampling::texture_sampling_point);
	depth_texture.create(w, h,
		TextureFormat::texture_format_float64,
		TextureSampling::texture_sampling_point);

	/* Step 2: Setup render config. */
	render_pass.eye.position = Vec3(0, 6, 10);
	render_pass.eye.look_at = Vec3(0, 3.5, 0);
	render_pass.eye.up_dir = Vec3(0, 1, 0);
	render_pass.eye.perspective.enabled = true;
	render_pass.eye.perspective.near = 1.0;
	render_pass.eye.perspective.far = 50.0;
	render_pass.eye.perspective.field_of_view = degrees_to_radians(60.0);
	render_pass.color_texture = &color_texture;
	render_pass.depth_texture = &depth_texture;
	render_pass.VS = model_VS;
	render_pass.FS = model_FS;

	/* Step 3: Initialize model. */
	boblamp_model.load("models/boblamp.zip");
	boblamp_model.dump();
	render_pass.model = &boblamp_model;

	pipeline.set_num_threads(4);
}

void 
render_frame(double T_global) {
	render_pass.time = fmod(T_global, 6.0);
	render_pass.anim_name = "";
	render_pass.eye.position = Vec3(10 * sin(T_global / 3), 6, 10 * cos(T_global / 3));
	render_pass.eye.look_at = Vec3(0, 3.5, 0);
	render_pass.run(pipeline);
}

int 
main(int argc, char* argv[]) {

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
      record_key(&e.key);

		/* timing */
    frameid++;
    frame_timer.tick();
    T_global += global_timer.tick();
    
    /* render the whole frame */
		render_frame(T_global);
        
		/* logging */
		double frame_time = frame_timer.tick();
		T_frame += frame_time;
    sgl::SDL2::sgl_texture_to_SDL_surface(render_pass.color_texture, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
		char buf[64];
		sprintf(buf, "%.2lfms, T=%.2lfs", T_frame / frameid * 1000.0, T_global);
    std::string title = std::string("SGL | ") + buf + " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

	destroy_env();
  return 0;
}
