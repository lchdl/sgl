#include "sgl_pipeline.h"
#include "unistd.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>

using namespace sgl;

int w = 320, h = 240;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

TextureLibrary texlib;
RenderConfig render_config;
Pipeline pipeline;
std::vector<Vertex> vertex_buffer;
std::vector<int32_t> index_buffer;
int colortex, depthtex;

void
RGBA8_texture_to_SDL_surface(const Texture* texture, SDL_Surface* surface) {
  uint8_t* src = (uint8_t*) texture->pixels;
  uint8_t* dst = (uint8_t*) surface->pixels;
  for (int y = 0; y < surface->h; y++) {
    for (int x = 0; x < surface->w; x++) {
      uint8_t r, g, b, a;
      int pid = y * w + x;
      dst[pid * 4 + 2] = src[pid * 4 + 0];
      dst[pid * 4 + 1] = src[pid * 4 + 1];
      dst[pid * 4 + 0] = src[pid * 4 + 2];
      dst[pid * 4 + 3] = src[pid * 4 + 3];
    }
  }
}

Vertex
build_vertex(Vec3 p) {
  Vertex v;
  v.n = normalize(p);
  v.p = p;
  v.t = Vec2();
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
  colortex = texlib.create_texture(render_width, render_height,
                                   TextureFormat::texture_format_RGBA8,
                                   TextureSampling::texture_sampling_point);
  depthtex = texlib.create_texture(render_width, render_height,
                                   TextureFormat::texture_format_float64,
                                   TextureSampling::texture_sampling_point);

  /* Step 2: Setup render config. */

  render_config.eye.position = Vec3(3, 3, 3);
  render_config.eye.look_at = Vec3(0, 0, 0);
  render_config.eye.up_dir = Vec3(0, 1, 0);
  render_config.eye.perspective.enabled = true;
  render_config.eye.perspective.near = 0.1;
  render_config.eye.perspective.far = 10.0;
  render_config.eye.perspective.field_of_view = PI / 3.0;

  render_config.textures[0] = texlib.get_texture(colortex);
  render_config.textures[1] = texlib.get_texture(depthtex);
  render_config.color_texture_id = 0;
  render_config.depth_texture_id = 1;
  render_config.uniform_texture_ids[0] = 0;
  render_config.model_transform = Mat4x4::identity();

  /* Step 3: Setup model. */
  build_model(vertex_buffer, index_buffer);

  Mat4x4 model_matrix;
  model_matrix = Mat4x4::identity();
}

int
main() {
  SDL_SetMainReady();

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);

  /* Create window */
  pWindow = SDL_CreateWindow("SGL Demo", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (pWindow == NULL)
    exit(1);

  /* Get window surface */
  pWindowSurface = SDL_GetWindowSurface(pWindow);

  init_pipeline();

  SDL_UpdateWindowSurface(pWindow);

  // Hack to get window to stay up
  SDL_Event e;
  Timer timer, frame_timer;
  bool quit = false;
  double T = 0.0;
  int frameid = 0;
  while (!quit) {
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT) {
      quit = true;
    }
    // const double slow_factor = 0.3;
    //  double T = timer.elapsed() * slow_factor;
    T += 0.01;
    frameid++;
    // render_config.eye.position = Vec3(3 * sin(T), 3, 3 * cos(T));
    render_config.eye.position = Vec3(3, 3, 3);
    pipeline.clear_textures(*texlib.get_texture(colortex),
                            *texlib.get_texture(depthtex),
                            Vec4(0.5, 0.5, 0.5, 1.0));
    frame_timer.tick();
    pipeline.hwspec.num_threads = 4;
    pipeline.rasterize(vertex_buffer, index_buffer, render_config);
    double frame_time = frame_timer.tick();
    RGBA8_texture_to_SDL_surface(texlib.get_texture(colortex), pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    std::string title = std::string("SGL Demo | ") + std::to_string(frameid) +
                        " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());

    // if (frameid == 24)
    //   __debugbreak();
  }

  return 0;
}
