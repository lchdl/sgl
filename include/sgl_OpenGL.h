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
#include "sgl_pass.h"
/**
Some functions defined here are used.
**/
#include "sgl_pipeline.h"

namespace sgl {
/* 
if OpenGL acceleration is enabled, we need to define an
enum to distinguish different devices.
*/
enum DeviceType {
  DeviceType_CPU,
  DeviceType_GPU,
};
 
namespace OpenGL {

/**
Initializes OpenGL under SDL2.
The SDL window must already be created before calling this function.

@param vsync Boolean flag to enable or disable vertical synchronization.

* Note: After calling this function, OpenGL will be initialized, and you will
  have access to all OpenGL API functions (e.g., `gl*`). Additionally, some
  utility classes are provided for convenience.
**/
bool initialize_OpenGL(SDL_Window* window, int major_version, int minor_version, bool vsync);

/**
Retrieves the size (width and height) of an OpenGL framebuffer object (FBO).
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
**/
IVec2 get_current_render_target_size();

class Texture : public sgl::Texture {
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
    to upload it to the GPU, you must manually call `Texture::to()`.
    If step (1) is omitted, SGL will only copy the texture data 
    available in CPU memory, which may be outdated if modifications 
    were made on the GPU. However, if you are certain that the CPU and
    GPU contain identical texture data, you can safely skip step (1).

  */
  bool to(sgl::DeviceType device);

  sgl::DeviceType get_device() const;
  const GLuint get_GL_handle() const;

protected:
  sgl::DeviceType device; /* where is the texture currently stored */
  GLuint gl_handle; /* OpenGL texture handle (0=invalid) */

public:
  /* reimplement base class function */
  void Texture::create(int32_t w, int32_t h, PixelFormat texture_format, TextureSampling texture_sampling, TextureUsage texture_usage);
  void destroy();

public:
  Texture();
  Texture(const sgl::Texture& source); /* convert from a plain texture */
  Texture(const Texture &texture);
  Texture &operator=(const Texture &texture);
  virtual ~Texture();
};

class Shader {
public:
  bool create(const std::string& vertex_source, const std::string& fragment_source);
  void use() const;
  GLuint get_GL_handle() const;
  GLint get_uniform_location(const std::string& name) const;

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

  void destroy();

  Shader();
  Shader(const std::string& vertexSource, const std::string& fragmentSource);
  /* disable copy */
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  virtual ~Shader();

protected:
  GLuint _compile_shader(GLenum type, const std::string& source);

protected:
  GLuint gl_handle;
};

struct VertexFormat { static void define_format() {} };
struct VertexFormat_3f3f2f : public VertexFormat { static void define_format(); };
struct VertexFormat_3f2f : public VertexFormat { static void define_format(); };
struct VertexFormat_2f2f : public VertexFormat { static void define_format(); };

template <typename VertexFormat_t>
class VertexBuffer {
public:
  void create_empty();
  void create_and_reserve(const int vertex_buffer_bytes, GLenum vertex_buffer_usage, const int index_buffer_bytes, GLenum index_buffer_usage);
  void create_and_fill(const GLsizei vertex_buffer_bytes, const void* vertex_data, GLenum vertex_buffer_usage, const GLsizei index_buffer_bytes, const void* index_data, GLenum index_buffer_usage);
  void subdata_VBO(GLintptr offset, GLsizeiptr size, const void* data); /* updates vertex array buffer (VBO) */
  void subdata_IBO(GLintptr offset, GLsizeiptr size, const void* data); /* updates element array buffer (IBO/EBO) */
  void draw_elements(GLenum mode, GLsizei count, GLenum type, const void *indices);
  void draw_arrays(GLenum mode, GLint first, GLsizei count);
  void destroy();

  VertexBuffer();
  virtual ~VertexBuffer();

protected:
  /**
  Fills the vertex buffer with vertex and index data.
  * Notes:
    - If vertex_buffer_bytes is set to 0, vertex buffer filling will be skipped.
    - If index_buffer_bytes is set to 0, index buffer filling will be skipped.
  **/
  void _realloc_and_fill(
    const GLsizei vertex_buffer_bytes, const void* vertex_data, GLenum vertex_buffer_usage,
    const GLsizei index_buffer_bytes, const void* index_data, GLenum index_buffer_usage);

public:
  GLuint get_VAO_GL_handle() const;
  GLuint get_VBO_GL_handle() const;
  GLuint get_IBO_GL_handle() const;

protected:
  GLuint VAO, VBO, IBO;
};

class FrameBuffer {
  /*
  The framebuffer currently only accepts color textures while handling depth 
  and stencil buffers internally without exposing them. This design decision 
  was made for several reasons:
  1. Depth and stencil buffers are used less frequently than color textures, 
     and exposing them would unnecessarily complicate library maintenance.
  2. The framebuffer primarily serves as an encapsulation of OpenGL framebuffer 
     objects. In OpenGL, depth buffers typically use 24-bit or float32 formats.
     However, in SGL, we store depth buffers in float64 format to match modern 
     CPU defaults. Supporting float32 would require additional effort without 
     providing significant benefits, as there's no compelling need to create 
     a float32 format solely for OpenGL compatibility.
  Consequently, we provide specialized methods only for transferring depth and 
  stencil buffer data when needed, while maintaining these buffers internally 
  within the framebuffer instance.
  */
public:
  void setup_color_attachment(sgl::OpenGL::Texture* tex, int slot); /* link color texture to framebuffer color texture slot */
  bool make();    /* assemble framebuffer, must done before binding */
  void destroy(); /* destroy framebuffer and return resources to system */
  void bind();    /* bind the framebuffer */
  void unbind();  /* unbind the framebuffer (bind default framebuffer) */

public:
  /* auxiliary functions */
  /* Blits (copies) a color component from a framebuffer attachment to the main framebuffer, automatically stretching to fill the full screen if dimensions differ. */
  void blit_color_attachment_to_main_framebuffer(int slot);
public:
  FrameBuffer();
  virtual ~FrameBuffer();
protected:
  sgl::OpenGL::Texture* color_slots[8];
  GLuint fbo;
  GLuint depth_stencil_texid;
  /* member variables for blitting framebuffer's content to main framebuffer (0) */
  sgl::OpenGL::Shader blit_shader; 
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_2f2f> quad_vbuf;
};

class SpriteRenderer {
protected:
  sgl::OpenGL::Shader shader;
  sgl::OpenGL::VertexBuffer<sgl::OpenGL::VertexFormat_2f2f> vbuf;
  SpriteOriginMode origin_mode;
public:
  void initialize();
  void destroy();
  void set_sprite_origin_mode(SpriteOriginMode mode);

  SpriteRenderer();
  virtual ~SpriteRenderer();
public:
  /**
  Renders a portion or the entirety of a sprite onto the screen.

  @param source: The source sprite to be rendered.
  @param target_w, target_h: The width and height of the render target 
                             (NOT the size of the destination sprite).
  @param src_x: The x-coordinate of the source sprite region to render.
  @param src_y: The y-coordinate of the source sprite region to render.
  @param src_w: The width of the source sprite region to render.
  @param src_h: The height of the source sprite region to render.
  @param dst_x: The x-coordinate of the destination location relative to the sprite origin.
  @param dst_y: The y-coordinate of the destination location relative to the sprite origin.
  @param scale: The scaling factor (x, y) to apply to the sprite.
  @param rot: The rotation angle (in degrees) to apply to the sprite.
  @param color_mask: A Vec3 representing the premultiplied color to apply to the sprite.
  @param origin_mode: The origin mode for rendering (see enum SpriteOriginMode for details).
  **/
  void draw(sgl::OpenGL::Texture* source, int target_w, int target_h,
    int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
    const Vec2& scale, const double& rot, const Vec3& color_mask,
    const sgl::SpriteOriginMode origin_mode = SpriteOriginMode_TopLeft);
};

/**
Blit texture (OpenGL version).
**/
void blit_texture(sgl::OpenGL::Texture* source, int target_w, int target_h,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const sgl::SpriteOriginMode origin_mode = SpriteOriginMode_TopLeft);

class Font : protected sgl::Font {

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
  std::map<uint8_t, sgl::OpenGL::Texture> page_id2tex;

};

/* the initialization process will also initialize the following states */
struct GL_states {
  GLint max_texture_image_units;     /* maximum number of textures that can be bound to a fragment shader */
  GLint max_color_attachments;
  SDL_Window* current_active_window; /* an `active` window refers to the window that currently holds the active OpenGL context. */
  SpriteRenderer sprite_renderer;

  GL_states() {
    max_texture_image_units = -1;
    max_color_attachments = -1;
    current_active_window = NULL;
  }
};

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::create_empty() {
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
inline void VertexBuffer<VertexFormat_t>::create_and_reserve(const int vertex_buffer_bytes, GLenum vertex_buffer_usage, const int index_buffer_bytes, GLenum index_buffer_usage)
{
  destroy();

  this->create_empty();
  this->_realloc_and_fill(vertex_buffer_bytes, NULL, vertex_buffer_usage, index_buffer_bytes, NULL, index_buffer_usage);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::create_and_fill(const GLsizei vertex_buffer_bytes, const void * vertex_data, GLenum vertex_buffer_usage, const GLsizei index_buffer_bytes, const void * index_data, GLenum index_buffer_usage)
{
  destroy();
  
  this->create_empty();
  this->_realloc_and_fill(vertex_buffer_bytes, vertex_data, vertex_buffer_usage, index_buffer_bytes, index_data, index_buffer_usage);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::_realloc_and_fill(const GLsizei vertex_buffer_bytes, const void * vertex_data, GLenum vertex_buffer_usage, const GLsizei index_buffer_bytes, const void * index_data, GLenum index_buffer_usage)
{
  if (VAO == 0) {
    printf("Error, vertex buffer is not initialized, cannot fill data.\n");
    return;
  }
  glBindVertexArray(VAO);
  if (vertex_buffer_bytes > 0) {
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_bytes, vertex_data, vertex_buffer_usage);
  }
  if (index_buffer_bytes > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_buffer_bytes, index_data, index_buffer_usage);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
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
  //glBindVertexArray(VAO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_elements(GLenum mode, GLsizei count, GLenum type, const void * indices) {
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
  glDrawElements(mode, count, type, indices);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

template<typename VertexFormat_t>
inline void VertexBuffer<VertexFormat_t>::draw_arrays(GLenum mode, GLint first, GLsizei count)
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

}; /* namespace sgl::OpenGL */
}; /* namespace sgl */

#endif /* #ifdef ENABLE_OPENGL */
