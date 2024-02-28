#include <stdio.h>

#include "sgl_SDL.h"
#include "sgl_pipeline.h"
#include "sgl_model.h"
#include "sgl_utils.h"

using namespace sgl;

int w = 800, h = 600;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

Pass render_pass;
Pipeline pipeline;
std::vector<Vertex> vertex_buffer;
std::vector<int32_t> index_buffer;
Texture colortex, depthtex, modeltex;

Vertex
build_vertex(Vec3 p) {
  Vertex v;
  v.p = p;
  v.n = normalize(p);
  v.t = Vec2(p.x + 1.0, p.z + 1.0) / 2.0;
  return v;
}

void
build_face(std::vector<int>& i, int i0, int i1, int i2) {
  i.push_back(i0);
  i.push_back(i1);
  i.push_back(i2);
}

void
build_model(std::vector<Vertex>& v, std::vector<int>& i) {
  using namespace sgl;
  v.clear();
  i.clear();
  v.push_back(build_vertex(Vec3(-1, -1, -1)));
  v.push_back(build_vertex(Vec3(-1, -1, +1)));
  v.push_back(build_vertex(Vec3(+1, -1, +1)));
  v.push_back(build_vertex(Vec3(+1, -1, -1)));
  v.push_back(build_vertex(Vec3(-1, +1, -1)));
  v.push_back(build_vertex(Vec3(-1, +1, +1)));
  v.push_back(build_vertex(Vec3(+1, +1, +1)));
  v.push_back(build_vertex(Vec3(+1, +1, -1)));
  build_face(i, 2, 1, 0);
  build_face(i, 2, 0, 3);
  build_face(i, 4, 5, 6);
  build_face(i, 4, 6, 7);

  build_face(i, 6, 5, 1);
  build_face(i, 6, 1, 2);
  build_face(i, 6, 2, 3);
  build_face(i, 6, 3, 7);

  build_face(i, 0, 7, 3);
  build_face(i, 0, 4, 7);
  build_face(i, 0, 1, 5);
  build_face(i, 0, 5, 4);
}

void
init_pipeline() {
  /* Step 1: Setup resources. */
  int render_width = w;
  int render_height = h;
  colortex.create(render_width, render_height,
                  TextureFormat::texture_format_RGBA8,
                  TextureSampling::texture_sampling_point);
  depthtex.create(render_width, render_height,
                  TextureFormat::texture_format_float64,
                  TextureSampling::texture_sampling_point);
  modeltex = load_texture("textures/checker.png");

  /* Step 2: Setup render config. */
  render_pass.eye.position = Vec3(3, 3, 3);
  render_pass.eye.look_at = Vec3(0, 0, 0);
  render_pass.eye.up_dir = Vec3(0, 1, 0);
  render_pass.eye.perspective.enabled = true;
  render_pass.eye.perspective.near = 0.1;
  render_pass.eye.perspective.far = 10.0;
  render_pass.eye.perspective.field_of_view = PI / 3.0;

  render_pass.color_texture = &colortex;
  render_pass.depth_texture = &depthtex;
  render_pass.in_textures[0] = &modeltex;
  render_pass.model_transform = Mat4x4::identity();

	pipeline.set_VS(vertex_shader);
	pipeline.set_FS(fragment_shader);

  /* Step 3: Setup model. */
  build_model(vertex_buffer, index_buffer);

  Mat4x4 model_matrix;
  model_matrix = Mat4x4::identity();
}

int
main(int argc, char* argv[]) {
  SDL_SetMainReady();

  set_cwd(gd(argv[0]));

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);

  /* Create window */
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (pWindow == NULL)
    exit(1);
  
  SDL_ShowWindow(pWindow);

  /* Get window surface */
  pWindowSurface = SDL_GetWindowSurface(pWindow);

  init_pipeline();

  SDL_UpdateWindowSurface(pWindow);

	//const aiScene* scene = sgl::Assimp::load_model("models/forest.zip");	
  Mesh mesh;
  mesh.load("models/forest.zip");

  printf("System initialized.\n");

  /* Start main loop */
  SDL_Event e;
  Timer timer, frame_timer;
  bool quit = false;
  double T = 0.0;
  int frameid = 0;
  const double time_factor = 0.3;
	double total_frame_time = 0.0;
	char buf[64];
  while (!quit) {
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = true;
    }
    T = timer.elapsed() * time_factor;
    frameid++;
		if (int(T/time_factor) % 2 == 0) {
			pipeline.set_FS(fragment_shader);
		}
		else {
			pipeline.set_FS(fragment_shader2);
		}
    render_pass.eye.position = Vec3(3 * sin(T), 3, 3 * cos(T));
    pipeline.clear_textures(*render_pass.color_texture,
                            *render_pass.depth_texture,
                            Vec4(0.5, 0.5, 0.5, 1.0));
    frame_timer.tick();
    pipeline.draw(vertex_buffer, index_buffer, render_pass);
    double frame_time = frame_timer.tick();
		total_frame_time += frame_time;
    sgl::SDL2::sgl_texture_to_SDL_surface(render_pass.color_texture, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
		sprintf(buf, "%.2lfms", total_frame_time / frameid * 1000.0);
    std::string title = std::string("SGL | ") + buf +
                        " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());

  }

  return 0;
}
