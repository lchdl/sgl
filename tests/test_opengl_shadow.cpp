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
  OpenGL::Shader shader_shadow, shader_main;
  OpenGL::Texture tex1, tex2;
  OpenGL::VertexBuffer<OpenGL::VertexFormat_3f2f> vbuf;

  OpenGL::FrameBuffer fbuf_render, fbuf_depth;
  OpenGL::Texture tex_color;
  OpenGL::Texture depth_attachment;

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

  gl.vbuf.create_and_fill(36, 5 * sizeof(float), vertices, GL_STATIC_DRAW, 0, 0, NULL, GL_STATIC_DRAW);
  
  sgl::OpenGL::Shader::FragDataLoc fs_outs[] = {
    {"FragDepth", 0},
  };
  gl.shader_shadow.create(R"(
    #version 330 core
    layout (location = 0) in vec3 inPosition;
    layout (location = 1) in vec2 inTexCoord;
    uniform mat4x4 Model;
    uniform mat4x4 LightTransform;
    void main()
    {
	    gl_Position = LightTransform * Model * vec4(inPosition, 1.0);
    }
    )",R"(
    #version 330 core
    layout(location = 0) out float FragDepth;
    void main()
    {
	    FragDepth = gl_FragCoord.z;
    }
    )",
    1, fs_outs
  );
  gl.shader_main.create(R"(
    #version 330 core
    layout (location = 0) in vec3 inPosition;
    layout (location = 1) in vec2 inTexCoord;
    uniform mat4x4 Model;
    uniform mat4x4 View;
    uniform mat4x4 Projection;
    uniform mat4x4 LightTransform;
    out vec2 TexCoord;
    out vec4 LightSpacePos;
    void main()
    {
	    gl_Position = Projection * View * Model * vec4(inPosition, 1.0);
	    TexCoord = inTexCoord;
      LightSpacePos = LightTransform * Model * vec4(inPosition, 1.0);
    }
    )", R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    in vec2 TexCoord;
    in vec4 LightSpacePos;
    uniform sampler2D tex1;
    uniform sampler2D tex2;
    uniform sampler2D tex_shadow;
    float shadow(vec4 LightSpacePos){
      /* Manually perform perspective divide and normalize coordinates to [0, 1]. */
      vec3 ShadowCoords = (LightSpacePos.xyz / LightSpacePos.w) * 0.5 + 0.5;
      /* Then sample depth and compare with current depth. */
      float ShadowDepth = texture(tex_shadow, ShadowCoords.xy).r; 
      float CurrentDepth = ShadowCoords.z;
      float ShadowBias = 0.005;
      float InShadow = CurrentDepth - ShadowBias > ShadowDepth ? 1.0 : 0.0;
      return InShadow;
    }
    void main()
    {
      float InShadow = shadow(LightSpacePos);
      vec4 ShadowCoeff = vec4(vec3(clamp(1.0 - InShadow, 0.5, 1.0)), 1.0);
	    FragColor = ShadowCoeff * mix(texture(tex1, TexCoord), texture(tex2, TexCoord), 0.5);
    }
    )"
  );

  gl.tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex2 = sgl::load_texture("assets/common/textures/brick/brick_diffuse_256.png", PixelFormat_RGBA8888, TextureSampling_Bilinear, true);
  gl.tex1.to_device(DeviceType_GPU);
  gl.tex2.to_device(DeviceType_GPU);

  /* init framebuffer here */
  gl.tex_color.create(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  gl.tex_color.to_device(DeviceType_GPU);
  gl.depth_attachment.create(w, h, PixelFormat_Float32, TextureSampling_Nearest, TextureUsage_DepthBuffer);
  gl.depth_attachment.to_device(DeviceType_GPU);
  gl.fbuf_render.setup_attachment(&gl.tex_color, 0);
  gl.fbuf_depth.setup_attachment(&gl.depth_attachment, 0);
  gl.fbuf_render.make();
  gl.fbuf_depth.make();

  gl.font.load("assets/common/fonts/Arial/11pt_Regular.fnt");
}

void render_scene(sgl::OpenGL::Shader& shader, double T) {
  const Vec3 model_locations[] = {
    {-0.5, 0.0, 0.0},
    {0.5, 1.5, 0.0},
    {2.0, 0.5, 0.0},
  };
  const double model_scalings[] = {
    1.0, 0.6, 1.5,
  };
  const Vec3 model_rotations[] = {
    {-1.0, 2.0, 3.0},
    {3.0, -2.0, 1.0},
    {2.0, 1.0, 1.0},
  };
  const int n_cubes = sizeof(model_locations) / sizeof(Vec3);
  for (int i = 0; i < n_cubes; i++) {
    Vec3 model_location = model_locations[i];
    double model_scaling = model_scalings[i];
    Vec3 model_rotation = model_rotations[i];
    Mat4x4 model = Mat4x4::translate(model_location.x, model_location.y, model_location.z) * Mat4x4::rotate(normalize(model_rotation), degrees_to_radians(T * 10.0)) * Mat4x4::scale(model_scaling, model_scaling, model_scaling);
    shader.set_uniform_matrix_4fv("Model", 1, GL_TRUE, &model);
    gl.vbuf.draw_arrays(GL_TRIANGLES, 0, 36);
  }
}

Mat4x4 shadow_pass(double T) {
  
  Mat4x4 light_xfm;
  
  gl.fbuf_depth.bind();
  {
    /* depth buffer should set to farthest 1.0f */
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
    const int w = rsize.x, h = rsize.y;

    const Vec3 light_pos = Vec3(8.0, 4.0, 2.0);
    const double range = 3.0, near_dist = 0.1, far_dist = 10.0;

    Mat4x4 light_view = sgl::get_view_matrix(light_pos, Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4x4 light_proj = sgl::get_orthographic_matrix(near_dist, far_dist, -range, +range, +range, -range);
    light_xfm = light_proj * light_view;

    gl.shader_shadow.use();
    gl.shader_shadow.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_xfm);
    render_scene(gl.shader_shadow, T);
  }
  gl.fbuf_depth.unbind();

  return light_xfm;
}

void main_pass(Mat4x4& light_transform, double T) {
  gl.fbuf_render.bind();
  {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
    const int w = rsize.x, h = rsize.y;

    Vec3 eye_pos = Vec3(3, 3, 3);

    Mat4x4 eye_view = sgl::get_view_matrix(eye_pos, Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4x4 eye_proj = sgl::get_perspective_matrix(double(w) / double(h), 0.1, 10.0, degrees_to_radians(60.0));

    gl.shader_main.use();
    gl.shader_main.set_uniform_matrix_4fv("LightTransform", 1, GL_TRUE, &light_transform);
    gl.shader_main.set_uniform_matrix_4fv("View", 1, GL_TRUE, &eye_view);
    gl.shader_main.set_uniform_matrix_4fv("Projection", 1, GL_TRUE, &eye_proj);
    gl.shader_main.set_texture_sampler_2D("tex1", gl.tex1, 0);
    gl.shader_main.set_texture_sampler_2D("tex2", gl.tex2, 1);
    gl.shader_main.set_texture_sampler_2D("tex_shadow", gl.depth_attachment, 2);
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
    main_pass(light_xfm, T);
  }
  gl.fbuf_render.blit_attachment_to_main_framebuffer(0, 0, 0, w, h);
  sgl::OpenGL::blit_texture(&gl.depth_attachment, w, h, 0, 0, w, h, 0, h, Vec2(0.4, 0.4), 0, Vec3(1, 1, 1), SpriteOriginMode_BottomLeft);
  gl.font.draw_text(L"Depth Buffer (near=0.0, far=1.0)", 1, h - 12, Vec4(0, 0, 0, 1));

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