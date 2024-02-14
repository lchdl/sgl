#include "tv_SDL.h"
#include <stdio.h>

#include "unistd.h"

#include "ppl_core.h"

SDL_Window *pWindow;
SDL_Surface *pWindowSurface;

int w = 256, h = 256;

void
run_pipeline() {

  /* Step 1: Setup resources. */
  int render_width  = w;
  int render_height = h;
  ppl::Texture color_surface;
  ppl::Texture depth_surface;
  color_surface.create(render_width, render_height, 4); /* RGBA8 per pixel */
  depth_surface.create(render_width, render_height, 8); /* float64 per pixel */

  /* Step 2: Setup camera parameters. */
  ppl::Vec3 eye_position = ppl::Vec3(0, 0, 0);
  ppl::Vec3 eye_look_at  = ppl::Vec3(0, 0, -1);
  ppl::Vec3 eye_up_dir   = ppl::Vec3(0, 1, 0);
  double near            = 0.99;
  double far             = 10.0;
  double field_of_view   = ppl::PI / 2.0;
  ppl::Pipeline pipeline;
  pipeline.setup_camera(eye_position, eye_look_at, eye_up_dir, near, far,
                        field_of_view);

  /* Step 3: Setup single triangle. */
  std::vector<ppl::Vertex> vertex_buffer;
  std::vector<int32_t> index_buffer;
  ppl::Triangle triangle;
  triangle.v[0].p = ppl::Vec3(0, 0, -1);
  triangle.v[1].p = ppl::Vec3(1, 0, -1);
  triangle.v[2].p = ppl::Vec3(1, 2, -1);
  triangle.v[0].n = ppl::Vec3(0, 0, 1);
  triangle.v[1].n = ppl::Vec3(0, 0, 1);
  triangle.v[2].n = ppl::Vec3(0, 0, 1);
  triangle.v[0].t = ppl::Vec2(0, 0);
  triangle.v[1].t = ppl::Vec2(1, 0);
  triangle.v[2].t = ppl::Vec2(1, 1);
  vertex_buffer.push_back(triangle.v[0]);
  vertex_buffer.push_back(triangle.v[1]);
  vertex_buffer.push_back(triangle.v[2]);
  index_buffer.push_back(0);
  index_buffer.push_back(1);
  index_buffer.push_back(2);

  ppl::Mat4x4 model_matrix;
  model_matrix = ppl::Mat4x4::identity();

  ppl::Texture in_texture = ppl::load_image_as_texture(
      "C:/Users/1/Desktop/LCH/tiny_vision/bin/checker.png");

  /* Step 3: Run pipeline */
  pipeline.clear_textures(color_surface, depth_surface,
                          ppl::Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.rasterize(vertex_buffer, index_buffer, model_matrix, color_surface,
                     depth_surface, &in_texture);

  /* Step 4: Update rendered surfaces. */
  memcpy(pWindowSurface->pixels, color_surface.pixels, 4 * w * h);
}

int
main() {

  SDL_SetMainReady();

  /* Initialize SDL */
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    exit(1);

  /* Create window */
  pWindow = SDL_CreateWindow("SDL", SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN);
  if (pWindow == NULL)
    exit(1);

  /* Get window surface */
  pWindowSurface = SDL_GetWindowSurface(pWindow);

  run_pipeline();

  /* Fill the surface gray */
  // SDL_FillRect(pWindowSurface, NULL,
  //              SDL_MapRGB(pWindowSurface->format, 0x80, 0x80, 0x80));
  SDL_UpdateWindowSurface(pWindow);

  // Hack to get window to stay up
  SDL_Event e;
  bool quit = false;
  while (!quit) {
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        quit = true;
    }
  }

  return 0;
}
