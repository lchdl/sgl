#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

struct Uniforms {
  Mat4x4 world, view, proj;
  Vec3 light_pos;
  Vec3 view_pos;
  int enable_normal_mapping;
  int enable_diffuse_texture;
  const Texture *diffuse;
  const Texture *normal;
  const Texture *specular;
};

struct VS_IN : public IVertex {
  Vec3 position; /* position */
  Vec3 normal; /* normal in model space */
  Vec2 texcoord; /* texcoords */
  Vec3 tangent; /* tangent in model space */
  Vec3 bitangent; /* bitangent in model space */

  VS_IN() {}
  VS_IN(Vec3 p, Vec3 n, Vec2 t, Vec3 tangent, Vec3 bitangent) { 
    this->position = p; this->normal = n; this->texcoord = t; 
    this->tangent = tangent;
    this->bitangent = bitangent;
  }
};

typedef struct VS_OUT : public IFragment {
  Vec3 world_pos;
  Vec2 texcoord;
  Vec3 tangent_light_pos;
  Vec3 tangent_view_pos;
  Vec3 tangent_frag_pos;

  VS_OUT() {}
  VS_OUT(Vec4 gl_Position, Vec3 world_pos, Vec2 texcoord, 
    Vec3 tangent_light_pos, Vec3 tangent_view_pos, Vec3 tangent_frag_pos) {
    this->gl_Position = gl_Position; 
    this->world_pos = world_pos; 
    this->texcoord = texcoord;
    this->tangent_frag_pos = tangent_frag_pos;
    this->tangent_light_pos = tangent_light_pos;
    this->tangent_view_pos = tangent_view_pos;
  }
  void operator*=(const double& scalar) {
    this->gl_Position *= scalar; 
    this->world_pos *= scalar;
    this->texcoord *= scalar;
    this->tangent_frag_pos *= scalar;
    this->tangent_light_pos *= scalar;
    this->tangent_view_pos *= scalar;
  }
  VS_OUT operator*(const double& scalar) const {
    return VS_OUT(this->gl_Position * scalar, this->world_pos * scalar, this->texcoord * scalar, 
      this->tangent_light_pos * scalar, this->tangent_view_pos * scalar, this->tangent_frag_pos * scalar);
  }
  VS_OUT operator+(const VS_OUT& frag) const {
    return VS_OUT(this->gl_Position + frag.gl_Position, this->world_pos + frag.world_pos, 
      this->texcoord + frag.texcoord, this->tangent_light_pos + frag.tangent_light_pos, 
      this->tangent_view_pos + frag.tangent_view_pos,
      this->tangent_frag_pos + frag.tangent_frag_pos);
  }
} FS_IN;

struct Shader {
  void VS(const Uniforms& uniforms, const VS_IN& vertex_in, VS_OUT& vertex_out, Vec4& gl_Position) const {
    Mat4x4 transform_WVP = mul(mul(uniforms.proj, uniforms.view), uniforms.world);
    gl_Position = mul(transform_WVP, Vec4(vertex_in.position, 1.0));
    vertex_out.texcoord = vertex_in.texcoord;
    vertex_out.world_pos = (uniforms.world * Vec4(vertex_in.position, 1.0)).xyz();
    /* 
    tangent, bitangent, and normal are in model space, we need to transform it to 
    world space by multiplying world matrix 
    */
    Vec3 T = normalize((uniforms.world * Vec4(vertex_in.tangent,   0.0)).xyz());
    Vec3 B = normalize((uniforms.world * Vec4(vertex_in.bitangent, 0.0)).xyz());
    Vec3 N = normalize((uniforms.world * Vec4(vertex_in.normal,    0.0)).xyz());
    Mat3x3 TBN = Mat3x3(T, B, N);
    /*
    Important note:
    Since GLSL uses column-major order when building matrices, the TBN matrix in GLSL 
    typically needs to be transposed for transforming vectors from world space to 
    tangent space. However, in this context, the TBN matrix is built using row-major 
    order, so no transposition is needed. The TBN matrix here can directly transform 
    any world space vector to tangent space.
    */
    vertex_out.tangent_light_pos = TBN * uniforms.light_pos;
    vertex_out.tangent_view_pos  = TBN * uniforms.view_pos;
    vertex_out.tangent_frag_pos  = TBN * vertex_out.world_pos;
  }
  void FS(const Uniforms& uniforms, const FS_IN& fragment_in, const Vec4& gl_FragCoord, FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const {
    Vec2 uv = fragment_in.texcoord;
    Vec3 color = Vec3(0.8, 0.8, 0.8);
    if (uniforms.enable_diffuse_texture)
      color = texture(uniforms.diffuse, uv).rgb();
    Vec3 tangent_normal = Vec3(0, 0, 1);
    if (uniforms.enable_normal_mapping)
      tangent_normal = normalize(texture(uniforms.normal, uv).rgb() * 2.0 - 1.0);
    Vec3 view_dir = normalize(fragment_in.tangent_view_pos - fragment_in.tangent_frag_pos);
    Vec3 light_dir = normalize(fragment_in.tangent_light_pos - fragment_in.tangent_frag_pos);

    double cos_theta = dot(tangent_normal, light_dir);
    double atten = clamp(0.0, cos_theta, 1.0);

    fs_outs[0] = Vec4(color * atten, 1.0);
  }
};

typedef Pipeline<Uniforms, VS_IN, FS_IN, Shader> Pipeline_t;

int w = 320, h = 240;
int num_threads = 2;
int tilt_model = 0;
//TextureSampling sampling = TextureSampling_Nearest;
TextureSampling sampling = TextureSampling_Bilinear;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window*         pWindow;
SDL_Surface* pWindowSurface;

Pipeline_t pipeline;
Pipeline_t::VertexBuffer_t vertices;
Pipeline_t::IndexBuffer_t   indices;
Model plane;

struct {
  Texture color;
  Texture depth;
} frame_buffer;
struct {
  Texture diffuse;
  Texture normal;
  Texture specular;
} brick;

Shader shader;
Uniforms uniforms;
Timer timer;

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
  if (keycode == SDLK_SPACE && is_press) {
    if (uniforms.enable_normal_mapping == 0) {
      uniforms.enable_normal_mapping = 1;
      printf("Normal mapping enabled.\n");
    }
    else{
      uniforms.enable_normal_mapping = 0;
      printf("Normal mapping disabled.\n");
    }
  }
  if (keycode == SDLK_a && is_press) {
    if (uniforms.enable_diffuse_texture == 0) {
      uniforms.enable_diffuse_texture = 1;
      printf("Diffuse texture enabled.\n");
    }
    else {
      uniforms.enable_diffuse_texture = 0;
      printf("Diffuse texture disabled.\n");
    }
  }
  if (keycode == SDLK_q && is_press) {
    if (tilt_model == 0) {
      tilt_model = 1;
      plane.set_model_transform(quat_to_mat3x3(Quat::rot_x(degrees_to_radians(20)) * Quat::rot_y(degrees_to_radians(20))));
    }
    else {
      tilt_model = 0;
      plane.set_model_transform(Mat4x4::identity());
    }
    printf("Model placement changed.\n");
  }
  if (keycode == SDLK_s && is_press) {
    if (sampling == TextureSampling_Nearest) {
      sampling = TextureSampling_Bilinear;
      printf("Use linear sampling.\n");
    }
    else {
      sampling = TextureSampling_Nearest;
      printf("Use point (nearest) sampling.\n");
    }
    brick.diffuse.set_sampling_mode(sampling);
    brick.normal.set_sampling_mode(sampling);
    brick.specular.set_sampling_mode(sampling);
  }
}

void init_render() {
  /* setup resources */
  frame_buffer.color = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  frame_buffer.depth = sgl::create_texture(w, h, PixelFormat_Float64, TextureSampling_Nearest, TextureUsage_DepthBuffer);
  brick.diffuse = sgl::load_texture("assets/standard/textures/brick/brick_diffuse_256.png");
  brick.normal = sgl::load_texture("assets/standard/textures/brick/brick_normal_256.png");
  brick.specular = sgl::load_texture("assets/standard/textures/brick/brick_specular_256.png");
  brick.diffuse.set_sampling_mode(sampling);
  brick.normal.set_sampling_mode(sampling);
  brick.specular.set_sampling_mode(sampling);

  /* setup pipeline */
  pipeline.bind_render_target(0, &frame_buffer.color);
  pipeline.bind_render_target(1, &frame_buffer.depth);
  pipeline.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.set_num_threads(num_threads);
  uniforms.enable_normal_mapping = 1;
  uniforms.enable_diffuse_texture = 1;
  tilt_model = 0;

  /* load model */
  plane.load_zip("assets/standard/models/plane.zip", "model.obj");
  auto convert_vertex_format = [](const Mesh::Vertex_t& v) {
    return VS_IN(v.position, v.normal, v.texcoord, v.tangent, v.bitangent);
  };
  const sgl::Mesh& mesh = plane.get_meshes()[0];
  for (int i=0; i < mesh.vertices.size(); i++) {
    vertices.push_back(
      convert_vertex_format(mesh.vertices[i]));
  }
  for (int i=0; i < mesh.indices.size(); i++) {
    indices.push_back(mesh.indices[i]);
  }
  printf("\n");
  printf("Press SPACE to enable/disable normal mapping.\n");
  printf("Press a to enable/disable diffuse texture.\n");
  printf("Press q to tilt/recover plane placement.\n");
  printf("Press s to use point/linear sampling.\n");
  printf("Press ESC to quit this demo.\n");
  printf("\n");
}

double render_frame(double T) {
  const double radius = 8.0;
  Vec3 pos  = Vec3(radius * sin(T / 3), 6, radius * cos(T / 3));
  Vec3 look = Vec3(0, 0, 0);
  Vec3 up   = Vec3(0, 1, 0);
  double aspect_ratio = double(frame_buffer.color.get_width()) / double(frame_buffer.color.get_height());
  uniforms.world = plane.get_model_transform();
  uniforms.view  = sgl::get_view_matrix(pos, look, up);
  uniforms.proj  = sgl::get_perspective_matrix(aspect_ratio, 0.1, 100.0, degrees_to_radians(60));    
  uniforms.diffuse = &brick.diffuse;
  uniforms.normal = &brick.normal;
  uniforms.specular = &brick.specular;
  uniforms.light_pos = Vec3(0, 2.5, 0);
  uniforms.view_pos = pos;

  pipeline.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  timer.tick();
  pipeline.draw(shader, vertices, indices, uniforms);
  return timer.tick();
}

int main(int argc, char* argv[]) {

  /* initialization */
  init_env(argc, argv);
  init_render();

  /* Start main loop */
  SDL_Event e;
  Timer global_timer;
  double T_global = 0.0, T_frame = 0.0;
  int frameid = 0;

  while (true) {
    /* window message handling */
    SDL_PollEvent(&e);
    if (e.type == SDL_QUIT || keystate[SDL_SCANCODE_ESCAPE])
      break;
    else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
      process_key(&e.key);

    /* render & timing */
    T_global += global_timer.tick();
    T_frame += render_frame(T_global);
    frameid++;

    /* logging */
    sgl::SDL2::sgl_texture_to_SDL2_surface(&frame_buffer.color, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 2) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}
