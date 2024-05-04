#include <stdio.h>

#include "sgl_SDL2.h"
#include "sgl_pipeline.h"
#include "sgl_model.h"
#include "sgl_utils.h"

using namespace sgl;

int w = 800, h = 600;

SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
bool keystate[SDL_NUM_SCANCODES];

Pipeline pipeline;
Uniforms uniforms;
std::vector<Vertex> vertices;
std::vector<int32_t> indices;

Texture colortex, depthtex;
Texture image;

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
init_render_pass() {
  /* Step 1: Setup resources. */
  colortex.create(w, h,
    TextureFormat::texture_format_RGBA8,
    TextureSampling::texture_sampling_point);
  depthtex.create(w, h,
    TextureFormat::texture_format_float64,
    TextureSampling::texture_sampling_point);

  Mat4x4 model(quat_to_mat3x3(Quat::rot_x(degrees_to_radians(-55.0))));
  Mat4x4 view(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, -3,
    0, 0, 0, 1
  );
  Mat4x4 projection = compute_projection_matrix(w, h, 0.1, 100, degrees_to_radians(45));
	image = sgl::load_texture("textures/checker_1024.png");
  uniforms.model = model;
  uniforms.view = view;
  uniforms.projection = projection;
	uniforms.in_textures[0] = &image;
  pipeline.set_render_targets(&colortex, &depthtex);
  pipeline.clear_render_targets(&colortex, &depthtex, Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.set_shaders(VS_default, FS_default);
  //pipeline.set_num_threads(4);
  pipeline.disable_backface_culling();
  
  double V[] = {
    // positions          // texture coords
     0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // top right
     0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
    -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
    -0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // top left 
  };
  unsigned int I[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };

  Vertex v;
  v.n = Vec3(0, 0, 1);
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

  indices.push_back(0);
  indices.push_back(1);
  indices.push_back(3);
  indices.push_back(1);
  indices.push_back(2);
  indices.push_back(3);
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
    pipeline.draw(vertices, indices, uniforms);

    double frame_time = frame_timer.tick();
    total_frame_time += frame_time;
    sgl::SDL2::sgl_texture_to_SDL_surface(&colortex, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    sprintf(buf, "%.2lfms", total_frame_time / frameid * 1000.0);
    std::string title = std::string("SGL | ") + buf +
      " | FPS=" + std::to_string(int(1.0 / frame_time));
    SDL_SetWindowTitle(pWindow, title.c_str());

  }

  return 0;
}
