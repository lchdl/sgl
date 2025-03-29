#include <stdio.h>
#include <sstream>

#ifdef ENABLE_OPENGL

/*

Variance shadow mapping (VSM).

Some useful tutorials:
  VSM fundamental explanation: https://www.youtube.com/watch?v=LqIl--GgfDA
  VSM tutorial [1/3]: https://www.youtube.com/watch?v=LGFDifcbsoQ
  VSM tutorial [2/3]: https://www.youtube.com/watch?v=F5QAkUloGOs
  VSM tutorial [3/3]: https://www.youtube.com/watch?v=mb7WuTDz5jw
  VSM results demo: https://graphics.stanford.edu/~mdfisher/Shadows.html

*/

#include "sgl.h"
#include "stb_image.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;
const int w = 480, h = 480;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

struct {
  OpenGL::Shader shader_VSM, shader_main;
  OpenGL::Texture tex1, tex2;
  OpenGL::VertexBuffer<OpenGL::VertexFormat_3f2f> vbuf;

  OpenGL::FrameBuffer fbuf_render, fbuf_VSM;
  OpenGL::Texture tex_color;
  OpenGL::Texture tex_VSM;       /* for variance shadow mapping (VSM) */

  /* for blurring the variance shadow map */
  OpenGL::FrameBuffer fbuf_blur; /* for blurring the VSM to achieve soft shadows */
  OpenGL::Texture tex_VSM_blur;  /* blurred variance shadow map */
  OpenGL::Shader shader_blur;

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
  using namespace sgl::OpenGL;

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
  
  Shader::FragDataLocation vsm_gen_outs[] = {
    {"FragVSM", 0},
  };
  gl.shader_VSM.create(
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_gen.vert"),
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_gen.frag"),
    sizeof(vsm_gen_outs) / sizeof(Shader::FragDataLocation), vsm_gen_outs
  );

  Shader::FragDataLocation vsm_main_outs[] = {
    {"FragColor", 0},
  };
  gl.shader_main.create(
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_main.vert"),
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_main.frag"),
    sizeof(vsm_main_outs) / sizeof(Shader::FragDataLocation), vsm_main_outs
  );

  gl.tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex2 = sgl::load_texture("assets/common/textures/brick/brick_diffuse_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex1.to_device(DeviceType_GPU);
  gl.tex2.to_device(DeviceType_GPU);

  gl.font.load("assets/common/fonts/Arial/11pt_Regular.fnt");

  /* init framebuffer here */
  gl.tex_color.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.tex_color.to_device(DeviceType_GPU);
  gl.tex_VSM.create(w, h, PixelFormat_OpenGL_RG32F, TextureSampling_Bilinear, TextureUsage_ColorComponents);
  gl.tex_VSM.to_device(DeviceType_GPU);
  gl.fbuf_render.setup_attachment(&gl.tex_color, 0);
  gl.fbuf_render.make();
  gl.fbuf_VSM.setup_attachment(&gl.tex_VSM, 0);
  gl.fbuf_VSM.make();

  gl.tex_VSM_blur.create(w, h, PixelFormat_OpenGL_RG32F, TextureSampling_Bilinear, TextureUsage_ColorComponents);
  gl.tex_VSM_blur.set_wrap_mode(TextureWrapMode_ClampToBorder);
  gl.tex_VSM_blur.set_border_color(Vec4(1, 1, 1, 1));
  gl.tex_VSM_blur.to_device(DeviceType_GPU);
  gl.fbuf_blur.setup_attachment(&gl.tex_VSM_blur, 0);
  gl.fbuf_blur.make();
  Shader::FragDataLocation vsm_blur_outs[] = {
    {"FragColor", 0},
  };
  gl.shader_blur.create(
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_blur.vert"),
    sgl::read_file_as_string("assets/common/shaders/test_opengl_VSM/vsm_blur.frag"),
    sizeof(vsm_blur_outs) / sizeof(Shader::FragDataLocation), vsm_blur_outs
  );
}

void render_scene(sgl::OpenGL::Shader& shader, double T) {
  const Vec3 model_locations[] = {{0, -10, 0}, {0, 0.5, 0}};
  const double model_scalings[] = {20, 1};
  const Vec3 model_rotations[] = {{0, 1, 0}, {3.0, -2.0, 1.0}};
  const double rotations_speeds[] = {0, 10.0};
  const int n_cubes = sizeof(model_locations) / sizeof(Vec3);
  for (int i = 0; i < n_cubes; i++) {
    Vec3 model_location = model_locations[i];
    double model_scaling = model_scalings[i];
    Vec3 model_rotation = model_rotations[i];
    double rotation_speed = rotations_speeds[i];
    Mat4x4 model = Mat4x4::translate(model_location.x, model_location.y, model_location.z) * Mat4x4::rotate(normalize(model_rotation), degrees_to_radians(T * rotation_speed)) * Mat4x4::scale(model_scaling, model_scaling, model_scaling);
    shader.set_uniform_matrix_4fv("Model", 1, GL_TRUE, &model);
    gl.vbuf.draw_arrays(GL_TRIANGLES, 0, 36);
  }
}

Mat4x4 shadow_pass(double T) {
  
  Mat4x4 light_xfm;
  
  gl.fbuf_VSM.bind();
  {
    /* depth buffer should set to farthest 1.0f */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
    const int w = rsize.x, h = rsize.y;

    const Vec3 light_pos = Vec3(2.0, 2.0, 2.0);
    const double range = 3.0, near_dist = 0.1, far_dist = 10.0;

    Mat4x4 light_view = sgl::get_view_matrix(light_pos, Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4x4 light_proj = sgl::get_orthographic_matrix(near_dist, far_dist, -range, +range, +range, -range);
    light_xfm = light_proj * light_view;

    gl.shader_VSM.use();
    gl.shader_VSM.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_xfm);
    render_scene(gl.shader_VSM, T);
  }
  gl.fbuf_VSM.unbind();

  return light_xfm;
}

void blur_VSM_pass() {
  gl.fbuf_blur.bind();
  {
    /* here note the custom shader instance we passed to blit_texture() */
    sgl::OpenGL::blit_texture(&gl.tex_VSM, w, h, 0, 0, w, h, 0, 0, Vec2(1, 1), 0, Vec3(1, 1, 1), SpriteOriginMode_TopLeft, &gl.shader_blur);
  }
  gl.fbuf_blur.unbind();
}

void main_pass(Mat4x4& light_transform, double T) {
  gl.fbuf_render.bind();
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
    const int w = rsize.x, h = rsize.y;

    Vec3 eye_pos = Vec3(3, 3, -3);

    Mat4x4 eye_view = sgl::get_view_matrix(eye_pos, Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4x4 eye_proj = sgl::get_perspective_matrix(double(w) / double(h), 0.1, 30.0, degrees_to_radians(60.0));

    gl.shader_main.use();
    gl.shader_main.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_transform);
    gl.shader_main.set_uniform_matrix_4fv("View", 1, GL_TRUE, &eye_view);
    gl.shader_main.set_uniform_matrix_4fv("Projection", 1, GL_TRUE, &eye_proj);
    gl.shader_main.set_texture_sampler_2D("tex1", gl.tex1, 0);
    gl.shader_main.set_texture_sampler_2D("tex2", gl.tex2, 1);
    gl.shader_main.set_texture_sampler_2D("tex_VSM", gl.tex_VSM_blur, 2);
    render_scene(gl.shader_main, T);
  }
  gl.fbuf_render.unbind();
}

double render_frame(double T) {
  sgl::Timer timer;
  timer.tick();

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  const int w = rsize.x, h = rsize.y;

  {
    Mat4x4 light_xfm;
    light_xfm = shadow_pass(T);
    blur_VSM_pass();
    main_pass(light_xfm, T);
  }
  gl.fbuf_render.blit_attachment_to_main_framebuffer(0, 0, 0, w, h);
  sgl::OpenGL::blit_texture(&gl.tex_VSM_blur, w, h, 0, 0, w, h, 0, h, Vec2(0.3,0.3), 0, Vec3(1, 1, 1), SpriteOriginMode_BottomLeft);
  gl.font.draw_text(L"Variance Shadow Map", 1, h - 12, Vec4(0, 0, 0, 1));

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