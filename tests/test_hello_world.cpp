#include <stdio.h>

#include "sgl.h"

using namespace sgl;

int w = 800, h = 600;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
bool keystate[SDL_NUM_SCANCODES];

Pipeline pipeline;
Uniforms uniforms;
std::vector<Vertex> vertices;
std::vector<int32_t> indices;

Texture color_texture, depth_texture;
Texture image_texture;

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
}

Mat4x4
compute_projection_matrix(double w, double h, double near, double far, double field_of_view) {
	double aspect_ratio = double(w) / double(h);
	double inv_aspect = double(1.0) / aspect_ratio;
	double left = -tan(field_of_view / double(2.0)) * near;
	double right = -left;
	double top = inv_aspect * right;
	double bottom = -top;
	return
		Mat4x4(2 * near / (right - left), 0, (right + left) / (right - left), 0,
			0, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0,
			0, 0, -(far + near) / (far - near),
			-2 * far * near / (far - near), 0, 0, -1.0, 0);
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

	/* rotate model along x axis by -55 degrees */
	Mat4x4 model(quat_to_mat3x3(Quat::rot_x(degrees_to_radians(-55.0))));
	/* translate model along z axis by -3 units */
	Mat4x4 view(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, -3,
		0, 0, 0, 1
	);
	/* near = 0.1, far = 10.0, field of view = 45 degrees */
	Mat4x4 projection = compute_projection_matrix(w, h, 0.1, 10.0, degrees_to_radians(45));
	
	/* initialize resources and render pipeline */
	image_texture = sgl::load_texture("textures/checker_256.png");
	uniforms.model = model;
	uniforms.view = view;
	uniforms.projection = projection;
	uniforms.in_textures[0] = &image_texture;
	pipeline.set_render_targets(&color_texture, &depth_texture);
	pipeline.clear_render_targets(&color_texture, &depth_texture, Vec4(0.5, 0.5, 0.5, 1.0));
	pipeline.set_shaders(default_VS, default_FS);
	pipeline.disable_backface_culling();

	Vertex v;
	/* 
	v.p: position (x,y,z)
	v.t: texture coords (u,v)
	*/
	v.p = Vec3(0.5, 0.5, 0.0);
	v.t = Vec2(1.0, 1.0);
	vertices.push_back(v);
	v.p = Vec3(0.5, -0.5, 0.0);
	v.t = Vec2(1.0, 0.0);
	vertices.push_back(v);
	v.p = Vec3(-0.5, -0.5, 0.0);
	v.t = Vec2(0.0, 0.0);
	vertices.push_back(v);
	v.p = Vec3(-0.5, 0.5, 0.0);
	v.t = Vec2(0.0, 1.0);
	vertices.push_back(v);

	/* triangle 1 */
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(3);
	/* triangle 2 */
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);
}

void
render_frame() {
	pipeline.draw(vertices, indices, uniforms);
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
			process_key(&e.key);

		/* timing */
		frameid++;
		frame_timer.tick();
		T_global += global_timer.tick();

		/* render the whole frame */
		render_frame();

		/* logging */
		double frame_time = frame_timer.tick();
		T_frame += frame_time;
		sgl::SDL2::sgl_texture_to_SDL_surface(&color_texture, pWindowSurface);
		SDL_UpdateWindowSurface(pWindow);
		char buf[64];
		sprintf(buf, "%.2lfms, T=%.2lfs", T_frame / frameid * 1000.0, T_global);
		std::string title = std::string("SGL | ") + buf + " | FPS=" + std::to_string(int(1.0 / frame_time));
		SDL_SetWindowTitle(pWindow, title.c_str());
	}

	destroy_env();
	return 0;
}
