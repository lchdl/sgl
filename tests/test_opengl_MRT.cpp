#include <stdio.h>
#include <sstream>

#ifdef ENABLE_OPENGL
#include "sgl.h"
#include "stb_image.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
const int w = 240, h = 240;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

struct {
  OpenGL::Shader shader_MRT;
  OpenGL::Texture tex1, tex2;
  OpenGL::VertexBuffer<OpenGL::VertexFormat_3f2f> vbuf;

  OpenGL::FrameBuffer framebuffer;
  OpenGL::Texture color_out0, color_out1, color_out2, color_out3;

  OpenGL::Font font;
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
  pWindow = SDL_CreateWindow("SGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w * 2, h, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
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
  
  sgl::OpenGL::Shader::FragDataLocation fs_outs[] = {
    {"FragColor", 0},
    {"FragDepth", 1},
    {"FragWorldPos", 2},
    {"FragTexCoord", 3},
  };
  gl.shader_MRT.create(R"(
    #version 330 core
    layout (location = 0) in vec3 inPosition;
    layout (location = 1) in vec2 inTexCoord;
    uniform mat4x4 model;
    uniform mat4x4 view;
    uniform mat4x4 projection;
    out vec2 TexCoord;
    out vec3 WorldPos;
    void main()
    {
	    gl_Position = projection * view * model * vec4(inPosition, 1.0);
	    TexCoord = inTexCoord;
      WorldPos = vec3(model * vec4(inPosition, 1.0));
    }
    )", R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    layout(location = 1) out vec4 FragDepth;
    layout(location = 2) out vec4 FragWorldPos;
    layout(location = 3) out vec4 FragTexCoord;
    in vec2 TexCoord;
    in vec3 WorldPos;
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    void main()
    {
      float d = clamp((gl_FragCoord.z - 0.9) * 10.0, 0.0, 1.0);
	    FragColor = mix(texture(tex1, TexCoord), texture(tex2, TexCoord), 0.5);
	    FragDepth = vec4(vec3(d), 1.0);
      FragWorldPos = clamp(2.0 * vec4(WorldPos, 1.0), 0.0, 1.0);
      FragTexCoord = vec4(TexCoord, 0.5, 1.0);
    }
    )",
    4, fs_outs
  );

  gl.tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex2 = sgl::load_texture("assets/common/textures/brick/brick_diffuse_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex1.to_device(DeviceType_GPU);
  gl.tex2.to_device(DeviceType_GPU);

  gl.color_out0.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out0.to_device(DeviceType_GPU);
  gl.color_out1.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out1.to_device(DeviceType_GPU);
  gl.color_out2.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out2.to_device(DeviceType_GPU);
  gl.color_out3.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.color_out3.to_device(DeviceType_GPU);
  gl.framebuffer.setup_attachment(&gl.color_out0, 0);
  gl.framebuffer.setup_attachment(&gl.color_out1, 1);
  gl.framebuffer.setup_attachment(&gl.color_out2, 2);
  gl.framebuffer.setup_attachment(&gl.color_out3, 3);
  gl.framebuffer.make();

  gl.font.load("assets/common/fonts/Arial/11pt_Regular.fnt");

}

void render_procedure(double T) {
  /* render to currently active framebuffer */

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  const int w = rsize.x, h = rsize.y;

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);

  Mat4x4 model = Mat4x4::rotate(normalize(Vec3(1, 2, 3)), degrees_to_radians(T * 30.0));
  Mat4x4 view = sgl::get_view_matrix(Vec3(1.1, 1.1, 1.1), Vec3(0, 0, 0), Vec3(0, 1, 0));
  Mat4x4 projection = sgl::get_perspective_matrix(double(w) / double(h), 0.1, 10.0, degrees_to_radians(60.0));

  gl.shader_MRT.use();
  gl.shader_MRT.set_texture_sampler_2D("tex1", gl.tex1, 0);
  gl.shader_MRT.set_texture_sampler_2D("tex2", gl.tex2, 1);
  gl.shader_MRT.set_uniform_matrix_4fv("model", 1, GL_TRUE, &model);
  gl.shader_MRT.set_uniform_matrix_4fv("view", 1, GL_TRUE, &view);
  gl.shader_MRT.set_uniform_matrix_4fv("projection", 1, GL_TRUE, &projection);
  gl.vbuf.draw_arrays(GL_TRIANGLES, 0, 36);

}

double render_frame(double T) {

  sgl::Timer timer;
  timer.tick();

  gl.framebuffer.bind();
  {
    render_procedure(T);
  }
  gl.framebuffer.unbind();
  gl.framebuffer.blit_attachment_to_main_framebuffer(0, 0, 0, w, h);
  gl.framebuffer.blit_attachment_to_main_framebuffer(1, w, 0, w, h);

  sgl::OpenGL::blit_texture(&gl.tex1, w * 2, h, 0, 0, gl.tex1.get_width(), gl.tex1.get_height(), w * 2, 0, Vec2(0.25, 0.25), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_TopRight);
  sgl::OpenGL::blit_texture(&gl.tex2, w * 2, h, 0, 0, gl.tex2.get_width(), gl.tex2.get_height(), w * 2, h, Vec2(0.25, 0.25), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_BottomRight);
  sgl::OpenGL::blit_texture(&gl.color_out2, w * 2, h, 0, 0, w, h, w, 0, Vec2(0.25, 0.25), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_TopLeft);
  sgl::OpenGL::blit_texture(&gl.color_out3, w * 2, h, 0, 0, w, h, w, h, Vec2(0.25, 0.25), 0.0, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_BottomLeft);

  gl.font.draw_text(L"WorldPos", w, 0, Vec4(1, 1, 1));
  gl.font.draw_text(L"TexCoord", w, h - 10, Vec4(1, 1, 1));

  int text_width;
  text_width = gl.font.get_text_extent_point(L"Main View").x;
  gl.font.draw_text(L"Main View", (w - text_width) / 2, 0, Vec4(1, 1, 1));
  text_width = gl.font.get_text_extent_point(L"Auxiliary View").x;
  gl.font.draw_text(L"Auxiliary View", w + (w - text_width) / 2, 0, Vec4(1, 1, 1));

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