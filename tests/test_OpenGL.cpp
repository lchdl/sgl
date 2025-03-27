#include <stdio.h>
#include <sstream>

#ifdef ENABLE_OPENGL
#include "sgl.h"
#include "stb_image.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
const int w = 480, h = 480;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

std::wstring long_text;

struct {
  OpenGL::Shader shader;
  OpenGL::Texture tex1, tex2, chess;
  OpenGL::VertexBuffer<OpenGL::VertexFormat_3f2f> vbuf;

  OpenGL::FrameBuffer framebuffer;
  OpenGL::Texture color_attachment;

  OpenGL::Font fonts[4];
} gl;

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
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (pWindow == NULL)
    exit(1);
  if (!sgl::OpenGL::initialize_OpenGL(pWindow, 3, 3, true))
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
}

void init_render() {

  float vertices[] = {
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
  };

  gl.vbuf.create_and_fill(sizeof(vertices), vertices, GL_STATIC_DRAW, 0, NULL, GL_STATIC_DRAW);
  
  gl.shader.create(
    sgl::read_file_as_string("assets/common/shaders/test.vert"),
    sgl::read_file_as_string("assets/common/shaders/test.frag")
  );

  gl.tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex2 = sgl::load_texture("assets/common/textures/brick/brick_diffuse_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.chess = sgl::load_texture("assets/common/textures/chess.png", PixelFormat_BGRA8888, TextureSampling_Nearest, true);
  gl.tex1.to_device(DeviceType_GPU);
  gl.tex2.to_device(DeviceType_GPU);
  gl.chess.to_device(DeviceType_GPU);

  /* init framebuffer here */
  gl.color_attachment.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_attachment.to_device(DeviceType_GPU);
  gl.framebuffer.setup_attachment(&gl.color_attachment, 0);
  gl.framebuffer.make();

  gl.fonts[0].load("assets/common/fonts/GrapeSoda/16pt_Regular.fnt");
  gl.fonts[1].load("assets/common/fonts/KiwiSoda/16pt_Regular.fnt");
  gl.fonts[2].load("assets/common/fonts/Catseye/16pt_Regular.fnt");
  gl.fonts[3].load("assets/common/fonts/MiniHerz/16pt_Regular.fnt");
  long_text = sgl::read_file_as_wstring("assets/common/texts/the_novel_of_ancient_Rome.txt");
  sgl::replace_all(long_text, L"\n", L"");
}

void render_procedure(double T) {
  /* render to currently active framebuffer */
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  const int w = rsize.x, h = rsize.y;

  Mat4x4 model = Mat4x4::rotate(normalize(Vec3(1, 2, 3)), degrees_to_radians(T * 30.0));
  Mat4x4 view = sgl::get_view_matrix(Vec3(1.5, 1.5, 1.5), Vec3(0, 0, 0), Vec3(0, 1, 0));
  Mat4x4 projection = sgl::get_perspective_matrix(double(w) / double(h), 0.1, 10.0, degrees_to_radians(60.0));

  gl.shader.use();
  gl.shader.set_texture_sampler_2D("texture1", gl.tex1, 0);
  gl.shader.set_texture_sampler_2D("texture2", gl.tex2, 1);
  gl.shader.set_uniform_matrix_4fv("model", 1, GL_TRUE, &model);
  gl.shader.set_uniform_matrix_4fv("view", 1, GL_TRUE, &view);
  gl.shader.set_uniform_matrix_4fv("projection", 1, GL_TRUE, &projection);
  gl.vbuf.draw_arrays(GL_TRIANGLES, 0, 36);

  /* Directly use this API to render a sprite to the screen without requiring any additional operations. */
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 81, 6, 14, 26, w / 2, h / 2 - 200, Vec2(2.0, 2.0), T, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_Center);
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 65, 8, 14, 24, w / 2, h / 2 + 200, Vec2(2.0, 2.0), -T, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_Center);
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 1, 16, 14, 16, 0, 0, Vec2(2.0, 2.0), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_TopLeft);
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 17, 12, 14, 20, 0, h, Vec2(2.0, 2.0), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_BottomLeft);
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 33, 13, 14, 19, w, h, Vec2(2.0, 2.0), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_BottomRight);
  sgl::OpenGL::blit_texture(&gl.chess, w, h, 49, 11, 14, 21, w, 0, Vec2(2.0, 2.0), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_TopRight);
}

double render_frame(double T) {

  sgl::Timer timer;
  timer.tick();

  gl.framebuffer.bind();
  {
    render_procedure(T);
    gl.fonts[0].draw_text(long_text, 30, 30, 120, 120, Vec4(1, 1, 1));
    gl.fonts[1].draw_text(long_text, w - 150, 30, 120, 120, Vec4(1, 1, 1));
    gl.fonts[2].draw_text(long_text, 30, h - 150, 120, 120, Vec4(1, 1, 1));
    gl.fonts[3].draw_text(long_text, w - 150, h - 150, 120, 120, Vec4(1, 1, 1));
  }
  gl.framebuffer.unbind();
  gl.framebuffer.blit_attachment_to_main_framebuffer(0, 0, 0, w, h);
 
  return timer.tick();
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

  /* Start main loop */
  SDL_Event e;
  sgl::Timer timer;

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* render & timing */
    T_global = timer.elapsed();
    T_frame += render_frame(T_global);
    frameid++;

    /* Swap buffers */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SDL_GL_SwapWindow(pWindow);
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 3) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}

#else
int main() {
  printf("OpenGL hardware acceleration (\"ENABLE_OPENGL\" flag) is disabled.\n");
}
#endif