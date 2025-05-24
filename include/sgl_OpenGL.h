#pragma once

#ifdef ENABLE_OPENGL

/*
This defines the sgl::OpenGL namespace, which provides a
hardware-accelerated alternative for common and essential
SGL drawing functions.

Some useful links:
* Initialize OpenGL in SDL2 library:
  https://lazyfoo.net/tutorials/SDL/51_SDL_and_modern_opengl/index.php

NOTE: The sgl library supports only a single OpenGL window. 
      Using multiple OpenGL windows is not supported.
*/

/*
The sgl::OpenGL namespace requires SDL2, so we need to
include its header.
*/
#include "sgl_SDL2.h"
#define GLEW_STATIC
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <gl/GLU.h>
/*
Inherits and extends the classes (providing a hardware
acceleration extension) defined in the header below.
*/
#include "sgl_primitives.h"
/**
Reimplement some passes with hardware acceleration 
defined in `sgl_pass.h`.
**/
#include "sgl_enums.h"
#include "sgl_pass.h"
#include "sgl_pipeline.h"
#include "sgl_model.h"

namespace sgl { 
namespace OpenGL {

/*
Useful function to check if a GL draw call is completed without error(s).
Available in OpenGL 4.3+.
*/
void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

/**
Initializes OpenGL under SDL2.
The SDL window must already be created before calling this function.

@param vsync Boolean flag to enable or disable vertical synchronization.

* Note: After calling this function, OpenGL will be initialized, and you will
  have access to all OpenGL API functions (e.g., `gl*`). Additionally, some
  utility classes are provided for convenience.
**/
bool initialize_OpenGL(SDL_Window* window, int major_version = 4, int minor_version = 3, bool vsync = false, bool debug = false);

bool is_OpenGL_initialized();

/**
Retrieves the size (width and height) of an OpenGL framebuffer object (FBO).
NOTE: Call this function before any gl* draw calls, as it may bind the
target framebuffer and reset some internal states, such as already bound
texture objects.
**/
IVec2 get_OpenGL_framebuffer_size(GLuint fbo, GLenum attachment = GL_COLOR_ATTACHMENT0);

/**
Retrieves the width and height of the current render target. 
This involves the following steps:
* If the current framebuffer is bound to 0 (the default framebuffer),
  the function uses `SDL_GL_GetCurrentContext()` to retrieve the active 
  window and obtain its size.
* Otherwise, if a framebuffer object (FBO) is bound, the function 
  retrieves the size of the framebuffer instead.
* If any error occurs during the process, a vector (-1, -1) is returned.
NOTE: Call this function before any gl* draw calls, as it may bind the 
target framebuffer and reset some internal states, such as already bound 
texture objects.
**/
IVec2 get_current_render_target_size(GLenum attachment = GL_COLOR_ATTACHMENT0);

GLuint get_current_framebuffer();

SDL_Window* get_SDL_window();

class Texture : protected sgl::Texture {
protected:
  sgl::DeviceType device; /* where is the texture currently stored */
  GLuint gl_handle; /* OpenGL texture handle (0=invalid) */
  sgl::OpenGL::TextureWrapMode wrap_mode;
  Vec4 border_color;
public:
  /*

  This function facilitates the transfer of a texture between devices, 
  typically between CPU host memory and GPU video memory. 

  * Note 1: For efficiency, when a texture is transferred from the CPU 
    to the GPU, the CPU retains a copy of the texture's previous state 
    before the transfer. However, users must refrain from accessing or 
    modifying the CPU's copy after the texture has been transferred to 
    the GPU, as this may lead to undefined behavior.

  * Note 2: When copying a texture to another texture, only the data 
    stored in CPU memory will be copied. This means that if a texture 
    is already on the GPU and you want to duplicate it, you need to:
    
    (1) Transfer the data from the GPU back to the CPU by calling 
      `Texture::to()`.
    (2) Create a new texture by copying the CPU-side data.
    
    The newly created texture will remain in CPU memory. If you need 
    to upload it to the GPU, you must manually call `Texture::to_devi
    ce()`. If step (1) is skipped, SGL will only copy the texture data 
    available in CPU memory, which may be outdated if modifications 
    were made on the GPU. However, if you are certain that the CPU and
    GPU contain identical texture data, you can safely skip step (1).

  * Note 3: If a texture has already been transferred to GPU and its 
    `=` operator is called (e.g., during value assignment from another 
    texture), this texture's GPU resources will be properly deallocated,
    and it will become a CPU texture after assignment.

  */
  bool to_device(sgl::DeviceType device, bool flip_vertically_on_transfer = false);

  sgl::DeviceType get_device() const;
  const GLuint get_GL_handle() const;

public:
  /* reimplement base class functions */
  void                    create(int32_t w, int32_t h, 
                                 PixelFormat texture_format, 
                                 TextureSampling texture_sampling, 
                                 TextureUsage texture_usage, 
                                 TextureWrapMode wrap_mode = TextureWrapMode_Repeat);
  void                   destroy();
  bool                  save_png(const std::string& path) const;
  void           flip_vertically();
  sgl::OpenGL::Texture to_format(const PixelFormat& target_format);
  int32_t              get_width() const;
  int32_t             get_height() const;
  int32_t    get_bytes_per_pixel() const;
  void*           get_pixel_data() const;
  PixelFormat   get_pixel_format() const;
  /* specific functions */
  void             set_wrap_mode(TextureWrapMode wrap_mode);
  TextureWrapMode  get_wrap_mode() const;
  void          set_border_color(const Vec4& border_color);

public:
  Texture();
  Texture(const sgl::Texture& source); /* convert from a plain texture */
  Texture(const Texture &texture);
  Texture &operator=(const Texture &texture);
  virtual ~Texture();
};

class Shader : public sgl::NonCopyable {
public:
  struct FragDataLoc {
    /* layout (location = `slot`) out `name`; */
    std::string name;
    GLuint slot;
  };
public:
  bool create(const std::string& vs, const std::string& fs);
  bool create(const std::string& vs, const std::string& fs, const int n_outs, const FragDataLoc* fs_outs); /* for multiple render targets (MRT) */
  void use() const;
  GLuint get_GL_handle() const;
  GLint get_uniform_location(const std::string& name) const;
  GLint get_frag_data_location(const std::string& name) const;

  bool set_uniform_1f(const std::string& name, GLfloat v0) const;
  bool set_uniform_2f(const std::string& name, GLfloat v0, GLfloat v1) const;
  bool set_uniform_3f(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2) const;
  bool set_uniform_4f(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const;
  bool set_uniform_1i(const std::string& name, GLint v0) const;
  bool set_uniform_2i(const std::string& name, GLint v0, GLint v1) const;
  bool set_uniform_3i(const std::string& name, GLint v0, GLint v1, GLint v2) const;
  bool set_uniform_4i(const std::string& name, GLint v0, GLint v1, GLint v2, GLint v3) const;
  bool set_uniform_1ui(const std::string& name, GLuint v0) const;
  bool set_uniform_2ui(const std::string& name, GLuint v0, GLuint v1) const;
  bool set_uniform_3ui(const std::string& name, GLuint v0, GLuint v1, GLuint v2) const;
  bool set_uniform_4ui(const std::string& name, GLuint v0, GLuint v1, GLuint v2, GLuint v3) const;
  bool set_uniform_1fv(const std::string& name, GLsizei count, const GLfloat* v) const;
  bool set_uniform_2fv(const std::string& name, GLsizei count, const GLfloat* v) const;
  bool set_uniform_3fv(const std::string& name, GLsizei count, const GLfloat* v) const;
  bool set_uniform_4fv(const std::string& name, GLsizei count, const GLfloat* v) const;
  bool set_uniform_matrix_2fv(const std::string& name, GLsizei count, GLboolean transpose, const GLfloat* v) const;
  bool set_uniform_matrix_3fv(const std::string& name, GLsizei count, GLboolean transpose, const GLfloat* v) const;
  bool set_uniform_matrix_4fv(const std::string& name, GLsizei count, GLboolean transpose, const GLfloat* v) const;
  bool set_uniform_matrix_2fv(const std::string& name, GLsizei count, GLboolean transpose, const sgl::Mat2x2* m) const;
  bool set_uniform_matrix_3fv(const std::string& name, GLsizei count, GLboolean transpose, const sgl::Mat3x3* m) const;
  bool set_uniform_matrix_4fv(const std::string& name, GLsizei count, GLboolean transpose, const sgl::Mat4x4* m) const;

  /*
  Binds a texture object with a texture sampler to a specific texture slot.
  * You can bind multiple textures to different samplers, as long as they occupy
    different texture slots.
  */
  bool set_texture_sampler_2D(const std::string& name, const sgl::OpenGL::Texture& texture, GLuint slot) const;
  bool set_texture_sampler_2D(const std::string& name, const GLuint tex_handle, GLuint slot) const;

  void destroy();

  Shader();
  Shader(const std::string& vs, const std::string& fs);
  Shader(const std::string& vs, const std::string& fs, const int n_outs, const FragDataLoc* fs_outs); /* for multiple render targets (MRT) */
  virtual ~Shader();

protected:
  GLuint _compile_shader(GLenum type, const std::string& source);

protected:
  GLuint gl_handle;
  std::vector<FragDataLoc> fs_outs;
};

struct VertexFormat { static void define_format() {} };
struct VertexFormat_3f3f2f : public VertexFormat { static void define_format(); };
struct VertexFormat_3f2f : public VertexFormat { static void define_format(); };
struct VertexFormat_2f2f : public VertexFormat { static void define_format(); };
struct VertexFormat_3f3f2f3f3f4i4f : public VertexFormat { static void define_format(); };

template <typename VertexFormat_t>
class VertexBuffer : public sgl::NonCopyable {
  /*
  The VertexBuffer class encapsulates an OpenGL Vertex Array Object (VAO), 
  a Vertex Buffer Object (VBO), and an Element Array Buffer (IBO). Being a 
  template class, it can adapt to various vertex data layouts.
  */
public:
  /*
  create_and_reserve(): creates the vertex buffer and not filling it.
  num_vertices: number of vertices of the vertex buffer being created.
  vertex_bytes: size (int number of bytes) of the each vertex element.
  vbuf_usage: vertex buffer usage (GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW)
  num_indices: number of indices
  index_element_bytes: index element size (for example, if each element is an int, set it to 4)
  ibuf_usage: index buffer usage (GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW)
  */
  void      create_and_reserve(int num_vertices, int vertex_bytes, GLenum vbuf_usage, 
                               int num_indices, int index_element_bytes, GLenum ibuf_usage);
  /*
  create_and_reserve(): creates the vertex buffer and not filling it.
  num_vertices: number of vertices of the vertex buffer being created.
  vertex_bytes: size (int number of bytes) of the each vertex element.
  vbuf_data: vertex buffer data pointer
  vbuf_usage: vertex buffer usage (GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW)
  num_indices: number of indices
  index_element_bytes: index element size (for example, if each element is an int, set it to 4)
  ibuf_data: index buffer data pointer
  ibuf_usage: index buffer usage (GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW)
  */
  void         create_and_fill(int num_vertices, int vertex_bytes, const void* vbuf_data, GLenum vbuf_usage, 
                               int num_indices, int index_element_bytes, const void* ibuf_data, GLenum ibuf_usage);
  /* 
  subdata_VBO(): updates vertex array buffer (VBO)
  offset: pointer offset (in bytes)
  size: number of modified bytes
  data: data pointer
  */
  void             subdata_VBO(GLintptr offset, GLsizeiptr size, const void* data); 
  /* 
  subdata_IBO(): updates element array buffer (IBO/EBO)
  offset: pointer offset (in bytes)
  size: number of modified bytes
  data: data pointer
  */
  void             subdata_IBO(GLintptr offset, GLsizeiptr size, const void* data); 
  void                 destroy();

  void           draw_elements(GLenum mode, GLsizei count, GLenum type, const void *indices) const;
  void             draw_arrays(GLenum mode, GLint first, GLsizei count) const;
  void draw_elements_instanced(GLenum mode, GLsizei count, GLenum type, const void *indices, const int num_instances) const;
  void   draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, const int num_instances) const;

  VertexBuffer();
  virtual ~VertexBuffer();

protected:
  void _create_empty();
  /**
  Fills the vertex buffer with vertex and index data.
  * Notes:
    - If vertex_buffer_bytes is set to 0, vertex buffer filling will be skipped.
    - If index_buffer_bytes is set to 0, index buffer filling will be skipped.
  **/
  void _realloc_and_fill(
    int num_vertices, int vertex_bytes, const void* vbuf_data, GLenum vbuf_usage,
    int num_indices, int index_element_bytes, const void* ibuf_data, GLenum ibuf_usage);

public:
  GLuint get_VAO_GL_handle() const;
  GLuint get_VBO_GL_handle() const;
  GLuint get_IBO_GL_handle() const;

  int get_num_vertices() const;
  int get_num_indices() const;

protected:
  GLuint VAO, VBO, IBO;
  int num_vertices, num_indices;
};

class FrameBuffer : public sgl::NonCopyable {
  /*
  The framebuffer currently supports only color textures as attachments,
  while managing depth and stencil buffers internally without exposing
  them. This design decision was made for the following reasons:

  1. Usage frequency: depth and stencil buffers are less commonly needed
     than color textures, and exposing them would add unnecessary
     complexity to library maintenance.

  2. Practical workaround: to access depth data, you can simply create a
     float32 texture, set it as a render target, and use multiple render
     targets (MRT) in the fragment shader to store gl_FragCoord.z in the
     desired output texture. This approach eliminates the need for a
     dedicated function to extract the internal depth buffer. The same
     logic applies to the stencil buffer.
  */
public:
  void setup_attachment(sgl::OpenGL::Texture* tex, int slot); /* link color texture to framebuffer color texture slot */
  bool            make(); /* assemble framebuffer, must done before binding */
  void         destroy(); /* destroy framebuffer and return resources to system */
  void            bind(); /* bind the framebuffer */
  void          unbind(); /* unbind the framebuffer (bind default framebuffer) */
  GLuint get_GL_handle() const;
  int        get_width() const;
  int       get_height() const;
public:
  /*
  Copies (blits) a color component from a framebuffer attachment to the main
  framebuffer.

  Important Notes:
  1. This operation will clear the currently bound framebuffer.
  2. For safety, the function includes a validation check. If a non-default
     framebuffer (ID != 0) is currently bound, the function will abort with
     an error and return immediately.
  */
  void blit_attachment_to_main_framebuffer(int slot, int dst_x, int dst_y, int dst_w, int dst_h);

public:
  FrameBuffer();
  virtual ~FrameBuffer();
protected:
  int w, h;
  sgl::OpenGL::Texture* color_slots[8];
  GLuint fbo;
  GLuint depth_stencil_texid;
  /* member variables for blitting framebuffer's content to main framebuffer (0) */
  sgl::OpenGL::Shader blit_shader;
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_2f2f> quad_vbuf;
};

class SpriteRenderer : public sgl::NonCopyable {
protected:
  sgl::OpenGL::Shader shader;
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_2f2f> vbuf;
public:
  void initialize(const std::string& vs = "", const std::string & fs = "", const int n_outs = 0, const Shader::FragDataLoc * fs_outs = NULL);
  void destroy();

  SpriteRenderer();
  virtual ~SpriteRenderer();
public:
  /**
  Renders a portion or the entirety of a sprite onto the screen.
  Parameters:
    source: The source sprite to be rendered.
    target_w, target_h: The width and height of the render target (NOT the size of the destination sprite).
    src_x, src_y, src_w, src_h: The source sprite region to render.
    dst_x, dst_y: The destination location relative to the sprite origin.
    scale: The scaling factor (x, y) to apply to the sprite.
    rot: The rotation angle (in degrees) to apply to the sprite.
    color_mask: A Vec3 representing the premultiplied color to apply to the sprite.
    origin_mode: The origin mode for rendering (see enum SpriteOriginMode for details).
  **/
  void draw(sgl::OpenGL::Texture* source, int target_w, int target_h,
    int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
    const Vec2& scale, const double& rot, const Vec3& color_mask,
    const sgl::SpriteOriginMode origin_mode = SpriteOriginMode_TopLeft,
    const sgl::OpenGL::Shader* custom_shader = NULL);
};

/**
Blit texture (OpenGL version).
**/
void blit_texture(sgl::OpenGL::Texture* source, int target_w, int target_h,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const sgl::SpriteOriginMode origin_mode = SpriteOriginMode_TopLeft,
  const sgl::OpenGL::Shader* custom_shader = NULL);

class Font : protected sgl::Font, public sgl::NonCopyable
{
public:
  /* 
  Batch processing of glyphs significantly improves rendering performance (~8.3x). 
  However, optimal batch size varies substantially across GPU architectures. This
  implementation adopts a median value that balances compatibility with mainstream 
  GPUs while maintaining performance gains, determined through empirical testing 
  on common hardware configurations.

  Benchmark results (lower is better):
  batch_size    render time (ms)
  -------------------------------------
  1             4.040 (no optimization)
  2             2.105
  4             1.248
  8             0.895
  16 (1KB buf)  0.685
  24            0.610
  32 (2KB buf)  0.568
  48            0.522
  64 (4KB buf)  0.485 (best value, ~8x)
  80            0.499
  96            0.496

  Observations:
  - Performance plateaus after batch size 64 (4KB buffer)
  - Larger batch sizes provide negligible performance improvements
  - Excessive batch sizes waste heap memory without meaningful gains
  Therefore, 64 has been selected as the optimal batch size for this implementation.
  */
  static const int BATCH_SIZE = 64; /* can be adjusted */
  static const int BATCH_BUFSIZE = Font::BATCH_SIZE * 16 * sizeof(float); /* do not adjust this */
public:
  bool load(const char* path);
  void unload();

  /* Renders text to the currently bound framebuffer. */
  IVec2 draw_text(const std::wstring & text, int x, int y, const Vec4& color);
  IVec2 draw_text(const std::wstring & text, int x, int y, int w, int h, const Vec4& color);

  IVec2 get_text_extent_point(const std::wstring & text);
  IVec2 get_text_extent_point(const std::wstring & text, int w, int h);

  void set_line_height(int new_height);

public:
  Font();
  virtual ~Font();

protected:
  sgl::OpenGL::Texture font_tex;
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_2f2f> vbuf;
  sgl::OpenGL::Shader shader;
  GLubyte* batch_buf;

};

class AnimatedModelRenderer : public sgl::NonCopyable {
  /*

  It appears that the `sgl::OpenGL::AnimatedModelRenderer` class should
  inherit from `sgl::AnimatedModelRenderer` since they share the same
  concept of storing complete model data for scene rendering. However,
  this inheritance was intentionally avoided for the following reasons:

  1. While sharing the same concept, `sgl::AnimatedModelRenderer`
     contains many data structures that
     `sgl::OpenGL::AnimatedModelRenderer` doesn't require.

  2. The complexity of `sgl::AnimatedModelRenderer` is already
     significant, and direct inheritance would exacerbate this,
     potentially making maintenance more difficult and introducing bugs.
     Since `sgl::OpenGL::AnimatedModelRenderer` also handles GPU
     interactions, inheriting from `sgl::AnimatedModelRenderer` might
     lead to incompatible member functions that could cause memory
     leaks or other hard-to-detect issues, making the code potentially
     unsafe.

     While protected inheritance with function hiding and rewriting
     could mitigate this, such an approach would be less maintainable
     than simply using composition.

  Therefore, each `sgl::OpenGL::AnimatedModelRenderer` contains a
  `sgl::Model` instance (`this->model`) as a member, accessing only the
  necessary data when required.

  Additionally, this class now supports instanced rendering of the same
  model, by setting instance count using this->set_num_instances()

  */
public:

  typedef VertexBuffer<VertexFormat_3f3f2f3f3f4i4f> VertexBuffer_t;

  typedef struct {
    float     position[3];
    float       normal[3];
    float     texcoord[2];
    float      tangent[3];
    float    bitangent[3];
    int       bone_IDs[4];
    float bone_weights[4];
  } Vertex_t;

  typedef struct {
    union {
      struct { float i[16]; };
      struct { float row0[4], row1[4], row2[4], row3[4]; };
    };
  } Mat4x4f;

  typedef struct {
    std::string anim_name; /* name of the current animation being played */
    double      play_time; /* time value for controlling the skeletal animation (in sec.) */
  } AnimInfo_t;

protected:
  sgl::Model* model;
  sgl::View* view;
  sgl::OpenGL::Shader shader;
  std::vector<VertexBuffer_t*> vbufs;
  std::map<void*, sgl::OpenGL::Texture*> texmap; /* Maps a texture's CPU memory pointer to its corresponding OpenGL texture. */

  std::vector<AnimInfo_t> inst_anims;
  /* 
  A shader storage buffer object used to store all model's bone matrices.
  NOTE: only available in OpenGL 4.3+.
  */
  GLuint bone_matrices_SSBO;
  GLuint model_matrices_SSBO;


protected:
  void          _resize_SSBOs(int new_count);

public:
  bool              set_model(sgl::Model* model, int num_instances = 1);
  void        set_view_params(sgl::View* view);
  void      set_num_instances(int count);
  void    set_model_transform(const Mat4x4& transform);
  void    set_model_transform(int instance_ID, const Mat4x4& transform);
  void    set_model_transform(const Vec3& pos, const Vec3& scale, const Vec3& rot_axis, const double& rot_angle);
  void    set_model_transform(int instance_ID, const Vec3& pos, const Vec3& scale, const Vec3& rot_axis, const double& rot_angle);
  void         play_animation(const std::string& anim_name, const double& play_time);
  void         play_animation(int instance_ID, const std::string& anim_name, const double& play_time);
  void                   draw(); /* draw all instances at once */
  int       get_num_instances() const;
  void                 unload();

  /*
  If you want to access detailed model members, use the following functions.
  */
  const std::vector<VertexBuffer_t*>& get_vertex_buffers() const;
  const sgl::Model* get_model() const;
  const std::map<void*, sgl::OpenGL::Texture*>& get_texmap() const;

public:
  AnimatedModelRenderer();
  virtual ~AnimatedModelRenderer();
};

struct GL_vars {
  /* the initialization process will also initialize the following states */
  GLint major_version, minor_version;

  GLint MAX_TEXTURE_IMAGE_UNITS;     /* maximum number of textures that can be bound to a fragment shader */
  GLint MAX_COLOR_ATTACHMENTS;

  SDL_Window* window; /* an `active` window refers to the window that currently holds the active OpenGL context. */

  SpriteRenderer sprite_renderer_RGBA;
  SpriteRenderer sprite_renderer_R32F;
  SpriteRenderer sprite_renderer_RG32F;

  bool ignore_minor_OpenGL_debug_messages;

  GL_vars();
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::_create_empty() {
  destroy();

  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &IBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  VertexFormat_t::define_format();
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::create_and_reserve(
  int num_vertices, int vertex_bytes, GLenum vbuf_usage,
  int num_indices, int index_element_bytes, GLenum ibuf_usage)
{
  destroy();

  this->_create_empty();
  this->_realloc_and_fill(num_vertices, vertex_bytes, NULL, vbuf_usage, num_indices * index_element_bytes, NULL, ibuf_usage);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::create_and_fill(
  int num_vertices, int vertex_bytes, const void* vbuf_data, GLenum vbuf_usage,
  int num_indices, int index_element_bytes, const void* ibuf_data, GLenum ibuf_usage)
{
  destroy();
  
  this->_create_empty();
  this->_realloc_and_fill(num_vertices, vertex_bytes, vbuf_data, vbuf_usage, num_indices, index_element_bytes, ibuf_data, ibuf_usage);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::_realloc_and_fill(
  int num_vertices, int vertex_bytes, const void* vbuf_data, GLenum vbuf_usage,
  int num_indices, int index_element_bytes, const void* ibuf_data, GLenum ibuf_usage)
{
  if (VAO == 0) {
    printf("Error, vertex buffer is not initialized, cannot fill data.\n");
    return;
  }
  glBindVertexArray(VAO);
  if (num_vertices * vertex_bytes > 0) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, num_vertices * vertex_bytes, vbuf_data, vbuf_usage);
  }
  if (num_indices * index_element_bytes > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, num_indices * index_element_bytes, ibuf_data, ibuf_usage);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  this->num_vertices = num_vertices;
  this->num_indices = num_indices;
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::subdata_VBO(GLintptr offset, GLsizeiptr size, const void * data)
{
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::subdata_IBO(GLintptr offset, GLsizeiptr size, const void * data)
{
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_elements(GLenum mode, GLsizei count, GLenum type, const void * indices) const
{
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawElements(mode, count, type, indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_arrays(GLenum mode, GLint first, GLsizei count) const
{
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawArrays(mode, first, count);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_elements_instanced(GLenum mode, GLsizei count, GLenum type, const void * indices, const int num_instances) const
{
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawElementsInstanced(mode, count, type, indices, num_instances);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_arrays_instanced(GLenum mode, GLint first, GLsizei count, const int num_instances) const
{
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawArraysInstanced(mode, first, count, num_instances);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::destroy() {
  if (VAO != 0) {
    glDeleteVertexArrays(1, &VAO);
    VAO = 0;
  }
  if (VBO != 0) {
    glDeleteBuffers(1, &VBO);
    VBO = 0;
  }
  if (IBO != 0) {
    glDeleteBuffers(1, &IBO);
    IBO = 0;
  }
  this->num_vertices = 0;
  this->num_indices = 0;
}

template<typename VertexFormat_t>
inline VertexBuffer<VertexFormat_t>::VertexBuffer() {
  VAO = 0;
  VBO = 0;
  IBO = 0;
}

template<typename VertexFormat_t>
inline VertexBuffer<VertexFormat_t>::~VertexBuffer() {
  destroy();
}

template<typename VertexFormat_t>
inline GLuint VertexBuffer<VertexFormat_t>::get_VAO_GL_handle() const { return VAO; }

template<typename VertexFormat_t>
inline GLuint VertexBuffer<VertexFormat_t>::get_VBO_GL_handle() const { return VBO; }

template<typename VertexFormat_t>
inline GLuint VertexBuffer<VertexFormat_t>::get_IBO_GL_handle() const { return IBO; }

template<typename VertexFormat_t>
inline int VertexBuffer<VertexFormat_t>::get_num_vertices() const
{
  return this->num_vertices;
}

template<typename VertexFormat_t>
inline int VertexBuffer<VertexFormat_t>::get_num_indices() const
{
  return this->num_indices;
}

}; /* namespace sgl::OpenGL */
}; /* namespace sgl */

#endif /* #ifdef ENABLE_OPENGL */
