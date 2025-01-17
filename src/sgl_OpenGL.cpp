#ifdef ENABLE_OPENGL

#include "sgl_OpenGL.h"

namespace sgl {
namespace OpenGL {

GL_states gl_states;

bool initialize_OpenGL(SDL_Window* window, int major_version, int minor_version, bool vsync)
{
  /*
  The OpenGL backend used by SGL relies on the SDL library. If the SDL 
  library is not linked with SGL, OpenGL acceleration will not be available.

  Below is a complete example of creating a window with OpenGL support in SDL:

  SDL_Window* pWindow = SDL_CreateWindow("OpenGL Example", 
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 
    SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (pWindow == NULL) 
    exit(1);
  bool vsync = false;
  if (!sgl::OpenGL::initialize_OpenGL(pWindow, vsync))
    exit(1);

  */

  if (gl_states.current_active_window != NULL) {
    printf("Error, OpenGL is already initialized.\n");
    return false;
  }

  /*
  Before creating the OpenGL context, it is essential to verify that "pWindow" 
  has been created with the "SDL_WINDOW_OPENGL" flag. If this flag is not set, 
  the user should be notified to recreate the window with the "SDL_WINDOW_OPENGL"
  flag enabled.
  */
  uint32_t window_flags = SDL_GetWindowFlags(window);
  if (!(window_flags & SDL_WINDOW_OPENGL)) {
    printf("The window is not created with \"SDL_WINDOW_OPENGL\" flag. Please re-create "
      "the window by adding the \"SDL_WINDOW_OPENGL\" flag and then try again.\n");
    printf("For example: SDL_Window* window = SDL_CreateWindow(... , ... | SDL_WINDOW_OPENGL);\n");
    return false;
  }

  /* Use OpenGL core profile */
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major_version);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor_version);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

  /* Create context */
  bool success = true;
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  if (gl_context == NULL) {
    printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
    return false;
  }
  else {
    /* Initialize GLEW */
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
      printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
      return false;
    }
    /* Use Vsync */
    if (vsync && SDL_GL_SetSwapInterval(1) < 0) {
      printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
      success = false;
    }
  }

  /*
  Initialize OpenGL states.
  */
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gl_states.max_texture_image_units);
  gl_states.current_active_window = window;
  gl_states.sprite_renderer.initialize();

  return success;
}

IVec2 get_OpenGL_framebuffer_size(GLuint fbo, GLenum attachment)
{
  GLint width, height;
  GLint type;
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Error, framebuffer is not complete!\n");
    width = height = -1;
  }
  else {
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    if (type == GL_TEXTURE) {
      GLint tex_id = 0;
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &tex_id);
      glBindTexture(GL_TEXTURE_2D, tex_id);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
      glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
    }
    else if (type == GL_RENDERBUFFER) {
      GLint rbo_id = 0;
      glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, attachment, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &rbo_id);
      glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
      glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    }
    else {
      printf("No valid attachment found for the given FBO.\n");
      width = height = -1;
    }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  return IVec2(width, height);
}

IVec2 get_current_render_target_size()
{
  GLint current_fbo = -1;
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &current_fbo);
  if (current_fbo == 0) {
    if (gl_states.current_active_window == NULL) {
      printf("Error, OpenGL is not initialized yet.\n");
      return IVec2(-1, -1);
    }
    IVec2 size;
    SDL_GL_GetDrawableSize(gl_states.current_active_window, &size.x, &size.y);
    return size;
  }
  else {
    return get_OpenGL_framebuffer_size(current_fbo, GL_COLOR_ATTACHMENT0);
  }

  return IVec2(-1, -1);
}

Texture::Texture() {
  device = DeviceType_CPU;
  gl_handle = 0;
}

Texture::Texture(const sgl::Texture& source) {
  device = DeviceType_CPU;
  gl_handle = 0;
  this->copy(source);
}

Texture::~Texture() {
  destroy();
}

Texture::Texture(const Texture &texture) {
  this->gl_handle = 0;
  this->device = DeviceType_CPU;
  this->copy(texture);
}

Texture& Texture::operator=(const Texture &texture) {
  if (this == &texture)
    return (*this);

  this->gl_handle = 0;
  this->device = DeviceType_CPU;
  copy(texture);

  return (*this);
}

bool Texture::to(sgl::DeviceType device)
{
  if (this->device == DeviceType_CPU && device == DeviceType_GPU) {
    /* Upload texture from CPU host memory to GPU VRAM. */
    if (gl_handle == 0) {
      /* Texture has not created yet, create it first. */
      glGenTextures(1, &gl_handle);
      if (gl_handle == 0) {
        printf("glGenTextures failed.\n");
        return false;
      }
      glBindTexture(GL_TEXTURE_2D, gl_handle);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      if (this->sampling == TextureSampling_Bilinear) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
      if (this->format == PixelFormat_RGBA8888) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->pixels);
      }
      else if (this->format == PixelFormat_BGRA8888) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, this->pixels);
      }
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    else {
      glBindTexture(GL_TEXTURE_2D, gl_handle);
      if (this->format == PixelFormat_RGBA8888) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->pixels);
      }
      else if (this->format == PixelFormat_BGRA8888) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, this->w, this->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, this->pixels);
      }
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    /* texture is uploaded to GPU, change its location */
    this->device = DeviceType_GPU;
    return true;
  }
  else if (this->device == DeviceType_GPU && device == DeviceType_CPU) {
    /* Download texture from GPU VRAM to CPU host memory. */
    if (gl_handle == 0) {
      printf("Cannot download texture data using an invalid OpenGL texture handle.\n");
      return false;
    }
    /* TODO: add support for transfering GPU data to CPU. */
  }
  return false;
}

sgl::DeviceType Texture::get_device() const
{
  return this->device;
}

const GLuint Texture::get_GL_handle() const
{
  return this->gl_handle;
}

void Texture::create(int32_t w, int32_t h, PixelFormat texture_format, TextureSampling texture_sampling, TextureUsage texture_usage) {
  sgl::Texture::create(w, h, texture_format, texture_sampling, texture_usage);
  device = DeviceType_CPU;
  gl_handle = 0;
}

void Texture::destroy() {
  device = DeviceType_CPU; /* default storage location is the CPU */
  glBindTexture(GL_TEXTURE_2D, 0);
  if (gl_handle > 0) {
    glDeleteTextures(1, &gl_handle);
    gl_handle = 0;
  }
  sgl::Texture::destroy();
}


bool Shader::create(const std::string & vertexSource, const std::string & fragmentSource) {
  destroy();

  GLuint vertexShader = _compile_shader(GL_VERTEX_SHADER, vertexSource);
  if (vertexShader == 0) {
    printf("Failed to compile vertex shader.\n");
    return false;
  }
  GLuint fragmentShader = _compile_shader(GL_FRAGMENT_SHADER, fragmentSource);
  if (fragmentShader == 0) {
    glDeleteShader(vertexShader);
    printf("Failed to compile fragment shader.\n");
    return false;
  }
  gl_handle = glCreateProgram();
  if (gl_handle == 0) {
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    printf("Failed to create shader program.\n");
    return false;
  }
  glAttachShader(gl_handle, vertexShader);
  glAttachShader(gl_handle, fragmentShader);
  glLinkProgram(gl_handle);
  GLint success;
  glGetProgramiv(gl_handle, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[2048];
    glGetProgramInfoLog(gl_handle, 2048, nullptr, infoLog);
    glDeleteProgram(gl_handle);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    printf("Shader program linking failed: \n%s", infoLog);
    return false;
  }
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);
  return true;
}

void Shader::destroy() {
  if (gl_handle != 0) {
    glDeleteProgram(gl_handle);
    gl_handle = 0;
  }
}

Shader::Shader() {
  gl_handle = 0;
}

Shader::Shader(const std::string & vertexSource, const std::string & fragmentSource) {
  gl_handle = 0;
  create(vertexSource, fragmentSource);
}

Shader::~Shader() {
  destroy();
}

void Shader::use() const {
  glUseProgram(gl_handle);
}

GLuint Shader::get_GL_handle() const {
  return gl_handle;
}

GLint Shader::get_uniform_location(const std::string & name) const {
  GLint location = glGetUniformLocation(gl_handle, name.c_str());
  if (location == -1) {
    printf("Warning: Uniform '%s' not found or not active.\n", name.c_str());
  }
  return location;
}

bool Shader::set_uniform_1f(const std::string & name, GLfloat v0) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform1f(location, v0);
  return true;
}
bool Shader::set_uniform_2f(const std::string & name, GLfloat v0, GLfloat v1) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform2f(location, v0, v1);
  return true;
}
bool Shader::set_uniform_3f(const std::string & name, GLfloat v0, GLfloat v1, GLfloat v2) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform3f(location, v0, v1, v2);
  return true;
}
bool Shader::set_uniform_4f(const std::string & name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform4f(location, v0, v1, v2, v3);
  return true;
}
bool Shader::set_uniform_1i(const std::string & name, GLint v0) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform1i(location, v0);
  return true;
}
bool Shader::set_uniform_2i(const std::string & name, GLint v0, GLint v1) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform2i(location, v0, v1);
  return true;
}
bool Shader::set_uniform_3i(const std::string & name, GLint v0, GLint v1, GLint v2) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform3i(location, v0, v1, v2);
  return true;
}
bool Shader::set_uniform_4i(const std::string & name, GLint v0, GLint v1, GLint v2, GLint v3) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform4i(location, v0, v1, v2, v3);
  return true;
}
bool Shader::set_uniform_1ui(const std::string & name, GLuint v0) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform1ui(location, v0);
  return true;
}
bool Shader::set_uniform_2ui(const std::string & name, GLuint v0, GLuint v1) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform2ui(location, v0, v1);
  return true;
}
bool Shader::set_uniform_3ui(const std::string & name, GLuint v0, GLuint v1, GLuint v2) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform3ui(location, v0, v1, v2);
  return true;
}
bool Shader::set_uniform_4ui(const std::string & name, GLuint v0, GLuint v1, GLuint v2, GLuint v3) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform4ui(location, v0, v1, v2, v3);
  return true;
}
bool Shader::set_uniform_1fv(const std::string& name, GLsizei count, const GLfloat* v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform1fv(location, count, v);
  return true;
}
bool Shader::set_uniform_2fv(const std::string& name, GLsizei count, const GLfloat* v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform2fv(location, count, v);
  return true;
}
bool Shader::set_uniform_3fv(const std::string& name, GLsizei count, const GLfloat* v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform3fv(location, count, v);
  return true;
}
bool Shader::set_uniform_4fv(const std::string& name, GLsizei count, const GLfloat* v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniform4fv(location, count, v);
  return true;
}
bool Shader::set_uniform_matrix_2fv(const std::string & name, GLsizei count, GLboolean transpose, const GLfloat * v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniformMatrix2fv(location, count, transpose, v);
  return true;
}
bool Shader::set_uniform_matrix_3fv(const std::string & name, GLsizei count, GLboolean transpose, const GLfloat * v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniformMatrix3fv(location, count, transpose, v);
  return true;
}
bool Shader::set_uniform_matrix_4fv(const std::string & name, GLsizei count, GLboolean transpose, const GLfloat * v) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  glUniformMatrix4fv(location, count, transpose, v);
  return true;
}
bool Shader::set_uniform_matrix_2fv(const std::string & name, GLsizei count, GLboolean transpose, const sgl::Mat2x2 * m) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  float* f = (float*)malloc(sizeof(float) * 4 * count);
  int p = 0;
  for (int n = 0; n < count; n++)
    for (int j = 0; j < 4; j++)
      f[p++] = float(m[n].i[j]);
  glUniformMatrix2fv(location, count, transpose, f);
  return true;
}
bool Shader::set_uniform_matrix_3fv(const std::string & name, GLsizei count, GLboolean transpose, const sgl::Mat3x3 * m) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  float* f = (float*)malloc(sizeof(float) * 9 * count);
  int p = 0;
  for (int n = 0; n < count; n++)
    for (int j = 0; j < 9; j++)
      f[p++] = float(m[n].i[j]);
  glUniformMatrix3fv(location, count, transpose, f);
  return true;
}
bool Shader::set_uniform_matrix_4fv(const std::string& name, GLsizei count, GLboolean transpose, const sgl::Mat4x4* m) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  float* f = (float*)malloc(sizeof(float) * 16 * count);
  int p = 0;
  for (int n = 0; n < count; n++)
    for (int j = 0; j < 16; j++)
      f[p++] = float(m[n].i[j]);
  glUniformMatrix4fv(location, count, transpose, f);
  return true;
}
bool Shader::set_texture_sampler_2D(const std::string& name, const sgl::OpenGL::Texture& texture, GLuint slot) const {
  if (int(slot) >= gl_states.max_texture_image_units) {
    printf("Invalid slot number given (%d), should <%d.\n", slot, gl_states.max_texture_image_units);
    return false;
  }
  if (!this->set_uniform_1i(name, slot))
    return false;
  if (texture.get_device() != DeviceType_GPU) {
    printf("Error, texture is not on GPU, transfer its data to GPU before continue.\n");
    return false;
  }
  if (texture.get_GL_handle() == 0) {
    printf("Error, texture handle is invalid.\n");
    return false;
  }
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture.get_GL_handle());
  return true;
}

GLuint Shader::_compile_shader(GLenum type, const std::string & source) {
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    printf("Failed to create shader object.\n");
    return 0;
  }
  const char* sourcePtr = source.c_str();
  glShaderSource(shader, 1, &sourcePtr, nullptr);
  glCompileShader(shader);
  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[2048];
    glGetShaderInfoLog(shader, 2048, nullptr, infoLog);
    glDeleteShader(shader);
    printf("Shader compilation failed:\n");
    printf("%s\n", infoLog);
    return 0;
  }
  return shader;
}

void VertexFormat_3f3f2f::define_format()
{
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
}

void VertexFormat_3f2f::define_format()
{
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

void VertexFormat_2f2f::define_format()
{
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
  glEnableVertexAttribArray(1);
}

bool Font::load(const char* path) {
  if(!sgl::Font::load(path)) 
    return false;

  /* reload textures here */

  FILE* fp = fopen(path, "r");
  if (fp == NULL) {
    printf("Cannot open file \"%s\".\n", path);
    return false;
  }

  auto regularize_string = [](std::string s) -> std::string {
    /*
    Replace multiple spaces with one space in a string, see
    https://stackoverflow.com/questions/8362094/replace-multiple-spaces-with-one-space-in-a-string
    */
    std::string::iterator new_end = std::unique(s.begin(), s.end(),
      [=](char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }
    );
    s.erase(new_end, s.end());
    /*
    In-place ltrim and rtrim, see
    https://stackoverflow.com/questions/216823/how-to-trim-a-stdstring
    */
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
  };

  auto read_config = [](std::string in, std::string& lhs, std::string& rhs) -> void {
    /*
    Read config defined with format "a=b".
    */
    std::vector<std::string> tokens = sgl::split(in, "=");
    lhs = tokens[0];
    rhs = tokens[1];
  };

  const int bufsize = 1024;
  char buf[bufsize];
  memset(buf, 0, bufsize);
  int lineno = 0;
  while (fgets(buf, bufsize - 1, fp) != NULL) {
    lineno++;
    std::string line = regularize_string(buf);
    std::vector<std::string> tokens = sgl::split(line, " ");
    std::string name, value;
    if (tokens[0] == "page") {
      int32_t page_id = -1;
      std::string page_fname = "";
      for (int itok = 1; itok < tokens.size(); itok++) {
        read_config(tokens[itok], name, value);
        if (name == "id") page_id = atoi(value.c_str());
        else if (name == "file") {
          sgl::replace_all(value, "\"", "");
          page_fname = value;
        }
      }
      std::string texdir = gd(path);
      std::string texfile = sgl::join(texdir, page_fname);
      sgl::OpenGL::Texture tex;
      if (!file_exists(texfile)) {
        printf("Cannot open file \"%s\", file not exist.\n", texfile.c_str());
      }
      else {
        tex = sgl::load_texture(texfile, PixelFormat_BGRA8888, TextureSampling_Nearest);
      }
      this->page_id2tex.insert_or_assign(page_id, tex);
    }
  }
  
  /* transfer to GPU */
  for (auto it = this->page_id2tex.begin(); it != this->page_id2tex.end(); ++it) {
    uint8_t page = it->first;
    sgl::OpenGL::Texture& texture = it->second;
    texture.to(DeviceType_GPU);
  }

  /* initialize shader */


  return true;
}

void Font::unload() {
  this->page_id2tex.clear();
  sgl::Font::unload();
}

Font::Font() {}

Font::~Font() {
  this->unload();
}

void Font::set_line_height(int new_height) {
  sgl::Font::set_line_height(new_height);
}

IVec2 Font::get_text_extent_point(const std::wstring & text) {
  return sgl::Font::get_text_extent_point(text);
}

IVec2 Font::get_text_extent_point(const std::wstring & text, int w, int h) {
  return sgl::Font::get_text_extent_point(text, w, h);
}

void SpriteRenderer::destroy() {
  this->shader.destroy();
  this->vbuf.destroy();
}

void SpriteRenderer::set_sprite_origin_mode(SpriteOriginMode mode) {
  float vbuf_data[24];
  this->origin_mode = mode;
  if (this->origin_mode == SpriteOriginMode_Center) {
    float vertices[16] = {
      -0.5f, -0.5f, +0.0f, +0.0f,
      +0.5f, -0.5f, +1.0f, +0.0f,
      +0.5f, +0.5f, +1.0f, +1.0f,
      -0.5f, +0.5f, +0.0f, +1.0f,
    };
    memcpy(vbuf_data, vertices, sizeof(vertices));
  }
  else if (this->origin_mode == SpriteOriginMode_BottomLeft) {
    float vertices[16] = {
      +0.0f, +0.0f, +0.0f, +0.0f,
      +1.0f, +0.0f, +1.0f, +0.0f,
      +1.0f, +1.0f, +1.0f, +1.0f,
      +0.0f, +1.0f, +0.0f, +1.0f,
    };
    memcpy(vbuf_data, vertices, sizeof(vertices));
  }
  else if (this->origin_mode == SpriteOriginMode_BottomRight) {
    float vertices[16] = {
      -1.0f, +0.0f, +0.0f, +0.0f,
      +0.0f, +0.0f, +1.0f, +0.0f,
      +0.0f, +1.0f, +1.0f, +1.0f,
      -1.0f, +1.0f, +0.0f, +1.0f,
    };
    memcpy(vbuf_data, vertices, sizeof(vertices));
  }
  else if (this->origin_mode == SpriteOriginMode_TopLeft) {
    float vertices[16] = {
      +0.0f, -1.0f, +0.0f, +0.0f,
      +1.0f, -1.0f, +1.0f, +0.0f,
      +1.0f, +0.0f, +1.0f, +1.0f,
      +0.0f, +0.0f, +0.0f, +1.0f,
    };
    memcpy(vbuf_data, vertices, sizeof(vertices));
  }
  else if (this->origin_mode == SpriteOriginMode_TopRight) {
    float vertices[16] = {
      -1.0f, -1.0f, +0.0f, +0.0f,
      +0.0f, -1.0f, +1.0f, +0.0f,
      +0.0f, +0.0f, +1.0f, +1.0f,
      -1.0f, +0.0f, +0.0f, +1.0f,
    };
    memcpy(vbuf_data, vertices, sizeof(vertices));
  }
  this->vbuf.vertex_buffer_subdata(0, sizeof(vbuf_data), vbuf_data);
}

SpriteRenderer::SpriteRenderer() {}

SpriteRenderer::~SpriteRenderer() {
  this->destroy();
}

void SpriteRenderer::draw(sgl::OpenGL::Texture * source, int target_w, int target_h, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, const Vec2 & scale, const double & rot, const Vec3 & color_mask, const sgl::SpriteOriginMode origin_mode)
{
  if (source->get_device() != DeviceType_GPU) {
    printf("Error, texture is not transferred to GPU.\n");
    return;
  }
  Mat4x4 scaling(
    scale.x, 0.0, 0.0, 0.0,
    0.0, scale.y, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(
    1.0, 0.0, 0.0, double(dst_x),
    0.0, 1.0, 0.0, double(dst_y),
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 rotation(quat_to_mat3x3(Quat::rot_z(rot)));
  Mat4x4 model = translation * rotation * scaling;
  Mat4x4 projection = sgl::get_orthographic_matrix(0.0, 1.0, 0.0, target_w, target_h, 0.0);
  Mat4x4 transform = projection * model;

  Vec2 src_sprite_size = Vec2(src_w, src_h);
  double du = 1.0 / double(source->get_width()), dv = 1.0 / double(source->get_height());
  float lx = float(du) * float(src_x), rx = float(du) * float(src_x + src_w);
  float by = float(dv) * float(source->get_height() - src_y - src_h), ty = float(dv) * float(source->get_height() - src_y);

  this->shader.use();
  this->shader.set_uniform_2f("src_sprite_size", float(src_w), float(src_h));
  this->shader.set_uniform_matrix_4fv("transform", 1, GL_TRUE, &transform);
  this->shader.set_uniform_3f("color_mask", float(color_mask.r), float(color_mask.g), float(color_mask.b));
  this->shader.set_texture_sampler_2D("texture1", *source, 0);

  if (origin_mode == SpriteOriginMode_Center) {
    float vertices[16] = {
      -0.5f, -0.5f, lx, by,
      +0.5f, -0.5f, rx, by,
      +0.5f, +0.5f, rx, ty,
      -0.5f, +0.5f, lx, ty,
    };
    this->vbuf.vertex_buffer_subdata(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_BottomLeft) {
    float vertices[16] = {
      +0.0f, +0.0f, lx, by,
      +1.0f, +0.0f, rx, by,
      +1.0f, +1.0f, rx, ty,
      +0.0f, +1.0f, lx, ty,
    };
    this->vbuf.vertex_buffer_subdata(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_BottomRight) {
    float vertices[16] = {
      -1.0f, +0.0f, lx, by,
      +0.0f, +0.0f, rx, by,
      +0.0f, +1.0f, rx, ty,
      -1.0f, +1.0f, lx, ty,
    };
    this->vbuf.vertex_buffer_subdata(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_TopLeft) {
    float vertices[16] = {
      +0.0f, -1.0f, lx, by,
      +1.0f, -1.0f, rx, by,
      +1.0f, +0.0f, rx, ty,
      +0.0f, +0.0f, lx, ty,
    };
    this->vbuf.vertex_buffer_subdata(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_TopRight) {
    float vertices[16] = {
      -1.0f, -1.0f, lx, by,
      +0.0f, -1.0f, rx, by,
      +0.0f, +0.0f, rx, ty,
      -1.0f, +0.0f, lx, ty,
    };
    this->vbuf.vertex_buffer_subdata(0, sizeof(vertices), vertices);
  }
  this->vbuf.draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void SpriteRenderer::initialize()
{
  shader.create(
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPosition;\n"
    "layout(location = 1) in vec2 aTexCoord;\n"
    "uniform mat4x4 transform;\n"
    "uniform vec2 src_sprite_size; /* size of the sprite (can be cropped) */\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "  gl_Position = transform * vec4(src_sprite_size * aPosition.xy, 0.0, 1.0);\n"
    "  TexCoord = aTexCoord;\n"
    "}\n"
    ,
    "#version 330 core\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D texture1;\n"
    "uniform vec3 color_mask;\n"
    "void main()\n"
    "{\n"
    "  gl_FragColor = texture(texture1, TexCoord) * vec4(color_mask, 1.0);\n"
    "}\n"
  );
  float vertices[] = {
    /* dummy data, just a placeholder */
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
  };
  int indices[] = {
    0, 1, 3,
    1, 2, 3,
  };
  vbuf.create();
  vbuf.alloc_buffer(sizeof(vertices), NULL, GL_DYNAMIC_DRAW, sizeof(indices), indices, GL_STATIC_DRAW); /* Index buffer will not be changed once set, so we set it to `GL_STATIC_DRAW`. */
}

void blit_texture(sgl::OpenGL::Texture* source, int target_w, int target_h,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const sgl::SpriteOriginMode origin_mode)
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  gl_states.sprite_renderer.draw(source, target_w, target_h, src_x, src_y, src_w, src_h, dst_x, dst_y, scale, rot, color_mask, origin_mode);
}

}; /* namespace OpenGL */
}; /* namespace sgl */

#endif
