#ifdef ENABLE_OPENGL

#include "sgl_OpenGL.h"

namespace sgl {
namespace OpenGL {

GL_vars gl_vars;

void GLAPIENTRY glDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
  /* Ignore non-significant codes */
  if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
  printf("--------------- GL DEBUG MESSAGE ---------------\n");
  printf("Debug message (err_id = %u): %s\n", id, message);
  switch (source)
  {
  case GL_DEBUG_SOURCE_API:               printf("Source: API");                break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:     printf("Source: Window System");      break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:   printf("Source: Shader Compiler");    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:       printf("Source: Third Party");        break;
  case GL_DEBUG_SOURCE_APPLICATION:       printf("Source: Application");        break;
  case GL_DEBUG_SOURCE_OTHER:             printf("Source: Other");              break;
  }
  printf("\n");
  switch (type)
  {
  case GL_DEBUG_TYPE_ERROR:               printf("Type: Error");                break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("Type: Deprecated Behaviour"); break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("Type: Undefined Behaviour");  break;
  case GL_DEBUG_TYPE_PORTABILITY:         printf("Type: Portability");          break;
  case GL_DEBUG_TYPE_PERFORMANCE:         printf("Type: Performance");          break;
  case GL_DEBUG_TYPE_MARKER:              printf("Type: Marker");               break;
  case GL_DEBUG_TYPE_PUSH_GROUP:          printf("Type: Push Group");           break;
  case GL_DEBUG_TYPE_POP_GROUP:           printf("Type: Pop Group");            break;
  case GL_DEBUG_TYPE_OTHER:               printf("Type: Other");                break;
  }
  printf("\n");
  switch (severity)
  {
  case GL_DEBUG_SEVERITY_HIGH:            printf("Severity: high");             break;
  case GL_DEBUG_SEVERITY_MEDIUM:          printf("Severity: medium");           break;
  case GL_DEBUG_SEVERITY_LOW:             printf("Severity: low");              break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:    printf("Severity: notification");     break;
  }
  printf("\n\n");
#if defined(DEBUG) || defined(_DEBUG)
  __debugbreak();
#endif
}

bool initialize_OpenGL(SDL_Window* window, int major_version, int minor_version, bool vsync, bool debug)
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

  if (gl_vars.current_active_window != NULL) {
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
  if (debug) {
    if (major_version >= 4 && minor_version >= 3) {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
    }
    else {
      printf("Warning: OpenGL debug context is only available in version 4.3 and above.\n");
    }
  }

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
    /* Check if debug context is actually created. */
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
      glEnable(GL_DEBUG_OUTPUT);
      glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
      glDebugMessageCallback(glDebugOutput, nullptr);
      glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
      printf("OpenGL debug context is enabled.\n");
    }
    else {
      if (debug) {
        printf("Error: OpenGL debug context cannot be created.\n");
      }
    }
  }

  /*
  Initialize OpenGL states.
  */
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &gl_vars.max_texture_image_units);
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &gl_vars.max_color_attachments);
  gl_vars.major_version = major_version;
  gl_vars.minor_version = minor_version;
  gl_vars.current_active_window = window;
  gl_vars.sprite_renderer_RGBA.initialize();
  gl_vars.sprite_renderer_R32F.initialize("", R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D tex0;
    uniform vec3 ColorMask;
    void main() {
      float color = texture(tex0, TexCoord).r; 
      FragColor = vec4(vec3(color), 1.0) * vec4(ColorMask, 1.0);
    }
    )", 1, NULL);
  gl_vars.sprite_renderer_RG32F.initialize("", R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D tex0;
    uniform vec3 ColorMask;
    void main() {
      vec2 color = texture(tex0, TexCoord).rg; 
      FragColor = vec4(vec2(color), 1.0, 1.0) * vec4(ColorMask, 1.0);
    }
    )", 1, NULL);

  return success;
}

IVec2 get_OpenGL_framebuffer_size(GLuint fbo, GLenum attachment)
{
  GLint width, height;
  GLint type;
  
  if (fbo == 0) {
    /* default framebuffer */
    if (gl_vars.current_active_window == NULL) {
      printf("Error, OpenGL is not initialized yet.\n");
      width = height = -1;
    }
    SDL_GL_GetDrawableSize(gl_vars.current_active_window, &width, &height);
  }
  else {
    /*
    If an FBO is currently bound when this function is called,
    we must store its ID and restore binding state after
    querying the target FBO's dimensions to avoid disrupting
    the rendering pipeline.
    */
    GLint current_fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
    if (current_fbo != fbo) /* avoid rebinding the same fbo */
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);

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
    if (current_fbo != fbo)
      glBindFramebuffer(GL_FRAMEBUFFER, current_fbo);
  }
  return IVec2(width, height);
}

IVec2 get_current_render_target_size(GLenum attachment)
{
  GLint current_fbo = -1;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
  return get_OpenGL_framebuffer_size(current_fbo, attachment);
}

GLuint get_current_framebuffer()
{
  GLint current_fbo = -1;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current_fbo);
  return GLuint(current_fbo);
}

Texture::Texture() {
  device = DeviceType_CPU;
  gl_handle = 0;
  wrap_mode = TextureWrapMode_Repeat;
}

Texture::Texture(const sgl::Texture& source) {
  device = DeviceType_CPU;
  gl_handle = 0;
  wrap_mode = TextureWrapMode_Repeat;
  this->copy(source);
}

Texture::~Texture() {
  destroy();
}

Texture::Texture(const Texture &texture) {
  this->gl_handle = 0;
  this->device = DeviceType_CPU;
  wrap_mode = TextureWrapMode_Repeat;
  this->copy(texture);
}

Texture& Texture::operator=(const Texture &texture) {
  if (this == &texture)
    return (*this);

  if (this->device != DeviceType_CPU) {
    this->destroy();
  }

  copy(texture);

  return (*this);
}

void Texture::set_wrap_mode(TextureWrapMode wrap_mode) {
  if (this->device != DeviceType_CPU) {
    printf("Warning: set_wrap_mode() will have no effect "
      "because the texture has already been uploaded to "
      "the device. The setting will take effect the next "
      "time to_device() is called.\n");
  }
  this->wrap_mode = wrap_mode;
}

TextureWrapMode Texture::get_wrap_mode() const {
  return this->wrap_mode;
}

void Texture::set_border_color(const Vec4& border_color) {
  if (this->device != DeviceType_CPU) {
    printf("Warning: set_border_color() will have no effect "
      "because the texture has already been uploaded to "
      "the device. The setting will take effect the next "
      "time to_device() is called.\n");
  }
  this->border_color = border_color;
}

bool Texture::to_device(sgl::DeviceType device, bool flip_vertically_on_transfer)
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
      if (this->wrap_mode == TextureWrapMode_Repeat) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      }
      else if (this->wrap_mode == TextureWrapMode_ClampToBorder) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = { float(border_color.r), float(border_color.g), float(border_color.b), float(border_color.a) };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
      }
      else if (this->wrap_mode == TextureWrapMode_ClampToEdge) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      else if (this->wrap_mode == TextureWrapMode_MirroredRepeat) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
      }
      if (this->sampling == TextureSampling_Bilinear) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else if (this->sampling == TextureSampling_Nearest) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }
      glGenerateMipmap(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    /* start data transfer */
    glBindTexture(GL_TEXTURE_2D, gl_handle);
    if (this->format == PixelFormat_RGBA8888) {
      if (flip_vertically_on_transfer) {
        sgl::OpenGL::Texture flipped = *this;
        flipped.flip_vertically();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, flipped.pixels);
      }
      else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->pixels);
      }
    }
    else if (this->format == PixelFormat_BGRA8888) {
      if (flip_vertically_on_transfer) {
        sgl::OpenGL::Texture flipped = *this;
        flipped.flip_vertically();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, flipped.pixels);
      }
      else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->w, this->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, this->pixels);
      }
    }
    else if (this->format == PixelFormat_Float32) {
      if (flip_vertically_on_transfer) {
        sgl::OpenGL::Texture flipped = *this;
        flipped.flip_vertically();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, this->w, this->h, 0, GL_RED, GL_FLOAT, flipped.pixels);
      }
      else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, this->w, this->h, 0, GL_RED, GL_FLOAT, this->pixels);
      }
    }
    else if (this->format == PixelFormat_OpenGL_RG32F) {
      if (flip_vertically_on_transfer) {
        sgl::OpenGL::Texture flipped = *this;
        flipped.flip_vertically();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, this->w, this->h, 0, GL_RG, GL_FLOAT, flipped.pixels);
      }
      else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, this->w, this->h, 0, GL_RG, GL_FLOAT, this->pixels);
      }
    }
    /* TODO: add support for other formats (GPU->CPU) */
    else {
      printf("Error, cannot transfer texture to GPU due to unsupported pixel format.\n");
      return false;
    }
    glGenerateMipmap(GL_TEXTURE_2D); /* regenerate mipmap since texture data is changed */
    glBindTexture(GL_TEXTURE_2D, 0);

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
    if (this->format == PixelFormat_BGRA8888 || this->format == PixelFormat_RGBA8888) {
      glBindTexture(GL_TEXTURE_2D, gl_handle);
      if (this->format == PixelFormat_BGRA8888)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, pixels);
      else if (this->format == PixelFormat_RGBA8888)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    else if (this->format == PixelFormat_Float32) {
      glBindTexture(GL_TEXTURE_2D, gl_handle);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, pixels);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    else if (this->format == PixelFormat_OpenGL_RG32F) {
      glBindTexture(GL_TEXTURE_2D, gl_handle);
      glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, pixels);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
    /* TODO: add support for other formats (GPU->CPU) */
    else {
      printf("Error, cannot transfer texture to CPU due to unsupported pixel format.\n");
      return false;
    }
    this->device = DeviceType_CPU;
    
    if (flip_vertically_on_transfer)
      this->flip_vertically();

    return true;
  }
  else if (this->device == DeviceType_CPU && device == DeviceType_CPU) {
    if (flip_vertically_on_transfer) {
      this->flip_vertically();
    }
    /* otherwise we do nothing here */
    return true;
  }
  else {
    printf("Unsupported device transfer. Current device enum id: %d, target device enum id: %d.\n", this->device, device);
    return false;
  }
  return false;
}

bool Texture::save_png(const std::string& path) const {
  /* perform additional check before saving */
  if (this->device != DeviceType_CPU) {
    printf("Error, texture is not on CPU host memory.\n");
    return false;
  }
  return sgl::Texture::save_png(path);
}

int32_t Texture::get_width() const { 
  return sgl::Texture::get_width(); 
}

int32_t Texture::get_height() const { 
  return sgl::Texture::get_height(); 
}

int32_t Texture::get_bytes_per_pixel() const { 
  return sgl::Texture::get_bytes_per_pixel();
}

void* Texture::get_pixel_data() const { 
  return sgl::Texture::get_pixel_data();
}

PixelFormat Texture::get_pixel_format() const { 
  return sgl::Texture::get_pixel_format();
}

void Texture::flip_vertically() {
  if (device != DeviceType_CPU) {
    printf("flip_vertically() will take no effect since texture is not on CPU host memory.\n");
    return;
  }
  sgl::Texture::flip_vertically();
}

Texture Texture::to_format(const PixelFormat& target_format) {
  /*
  If the texture object has been transferred to the GPU, we need to:
  1. Transfer it back to CPU memory;
  2. Release the GPU-allocated resources.
 
  Reason: The texture format on the CPU side has been modified, requiring 
  synchronization with the GPU version. However, implementing this 
  synchronization automatically would introduce significant complexity. 
  Therefore, we delegate this responsibility to the calling code and let 
  users handle the synchronization explicitly.
  */
  if (this->device != DeviceType_CPU) {
    this->to_device(DeviceType_CPU);
    glDeleteTextures(1, &this->gl_handle);
    this->gl_handle = 0;
  }
  return sgl::Texture::to_format(target_format);
}

sgl::DeviceType Texture::get_device() const
{
  return this->device;
}

const GLuint Texture::get_GL_handle() const
{
  return this->gl_handle;
}

void Texture::create(int32_t w, int32_t h, PixelFormat texture_format, TextureSampling texture_sampling, TextureUsage texture_usage, TextureWrapMode wrap_mode) {
  this->destroy();
  sgl::Texture::create(w, h, texture_format, texture_sampling, texture_usage);
  device = DeviceType_CPU;
  gl_handle = 0;
  this->wrap_mode = wrap_mode;
}

void Texture::destroy() {
  device = DeviceType_CPU; /* default storage location is the CPU */
  wrap_mode = TextureWrapMode_Repeat;
  glBindTexture(GL_TEXTURE_2D, 0);
  if (gl_handle > 0) {
    glDeleteTextures(1, &gl_handle);
    gl_handle = 0;
  }
  sgl::Texture::destroy();
}

bool Shader::create(const std::string & vs, const std::string & fs) {
  return this->create(vs, fs, 0, NULL);
}

bool Shader::create(const std::string & vs, const std::string & fs, const int n_outs, const FragDataLocation * fs_outs)
{
  destroy();

  GLuint vertexShader = _compile_shader(GL_VERTEX_SHADER, vs);
  if (vertexShader == 0) {
    printf("Failed to compile vertex shader.\n");
    return false;
  }
  GLuint fragmentShader = _compile_shader(GL_FRAGMENT_SHADER, fs);
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

  for (size_t i = 0; i < n_outs; i++) {
    glBindFragDataLocation(gl_handle, fs_outs[i].slot, fs_outs[i].name.c_str());
    this->fs_outs.push_back(fs_outs[i]);
  }

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
  fs_outs.clear();
  fs_outs.shrink_to_fit();
}

Shader::Shader() {
  gl_handle = 0;
}

Shader::Shader(const std::string & vs, const std::string & fs) {
  gl_handle = 0;
  create(vs, fs);
}

Shader::Shader(const std::string & vs, const std::string & fs, const int n_outs, const FragDataLocation * fs_outs)
{
  gl_handle = 0;
  create(vs, fs, n_outs, fs_outs);
}

Shader::~Shader() {
  destroy();
}

void Shader::use() const {
  if (this->gl_handle == 0) {
    printf("Error, shader handle is invalid.\n");
    return;
  }
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

GLint Shader::get_frag_data_location(const std::string & name) const {
  return glGetFragDataLocation(this->gl_handle, name.c_str());
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
  float f[4];
  for (int n = 0; n < count; n++) {
    for (int j = 0; j < 4; j++) {
      f[j] = float(m[n].i[j]);
    }
    if ((location = get_uniform_location(name + '[' + std::to_string(n) + ']')) < 0)
      return false;
    glUniformMatrix2fv(location, 1, transpose, f);
  }
  return true;
}
bool Shader::set_uniform_matrix_3fv(const std::string & name, GLsizei count, GLboolean transpose, const sgl::Mat3x3 * m) const {
  GLint location;
  if ((location = get_uniform_location(name)) < 0)
    return false;
  float f[9];
  for (int n = 0; n < count; n++) {
    for (int j = 0; j < 9; j++) {
      f[j] = float(m[n].i[j]);
    }
    if ((location = get_uniform_location(name + '[' + std::to_string(n) + ']')) < 0)
      return false;
    glUniformMatrix3fv(location, 1, transpose, f);
  }
  return true;
}
bool Shader::set_uniform_matrix_4fv(const std::string& name, GLsizei count, GLboolean transpose, const sgl::Mat4x4* m) const {
  GLint location;
  float f[16];
  for (int n = 0; n < count; n++) {
    for (int j = 0; j < 16; j++) {
      f[j] = float(m[n].i[j]);
    }
    if ((location = get_uniform_location(name + '[' + std::to_string(n) + ']')) < 0)
      return false;
    glUniformMatrix4fv(location, 1, transpose, f);
  }
  return true;
}
bool Shader::set_texture_sampler_2D(const std::string& name, const sgl::OpenGL::Texture& texture, GLuint slot) const {
  if (int(slot) >= gl_vars.max_texture_image_units) {
    printf("Invalid slot number given (%d), should <%d.\n", slot, gl_vars.max_texture_image_units);
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

bool Shader::set_texture_sampler_2D(const std::string & name, const GLuint tex_handle, GLuint slot) const
{
  if (int(slot) >= gl_vars.max_texture_image_units) {
    printf("Invalid slot number given (%d), should <%d.\n", slot, gl_vars.max_texture_image_units);
    return false;
  }
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, tex_handle);
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

void VertexFormat_3f3f2f3f3f4i4f::define_format()
{
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)(8 * sizeof(float)));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)(11 * sizeof(float)));
  glEnableVertexAttribArray(4);
  glVertexAttribIPointer(5, 4, GL_INT, 18 * sizeof(float) + 4 * sizeof(int), (void*)(14 * sizeof(float))); /* note the IPointer used here */
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, 18 * sizeof(float) + 4 * sizeof(int), (void*)(14 * sizeof(float) + 4 * sizeof(int)));
  glEnableVertexAttribArray(6);
}


bool Font::load(const char* path) {

  unload();

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
      if (page_id != 0) {
        printf("Error, font rendering does not support glyph stored in multiple pages.\n");
        fclose(fp);
        unload();
        return false;
      }
      std::string texdir = gd(path);
      std::string texfile = sgl::join(texdir, page_fname);
      sgl::OpenGL::Texture tex;
      if (!file_exists(texfile)) {
        printf("Cannot open file \"%s\", file not exist.\n", texfile.c_str());
      }
      else {
        font_tex = sgl::load_texture(texfile, PixelFormat_BGRA8888, TextureSampling_Nearest, true);
      }
    }
  }
  
  font_tex.to_device(DeviceType_GPU);

  /* create VBO and shader for rendering */
  int sizeof_indices = sizeof(int) * 6 * Font::batch_size;
  int* indices = (int*)malloc(sizeof_indices);
  for (int i = 0; i < Font::batch_size; i++) {
    indices[i * 6 + 0] = 0 + i * 4;
    indices[i * 6 + 1] = 1 + i * 4;
    indices[i * 6 + 2] = 3 + i * 4;
    indices[i * 6 + 3] = 1 + i * 4;
    indices[i * 6 + 4] = 2 + i * 4;
    indices[i * 6 + 5] = 3 + i * 4;
  }
  vbuf.create_and_fill(Font::batch_bufsz, NULL, GL_DYNAMIC_DRAW, sizeof_indices, indices, GL_STATIC_DRAW); /* Index buffer will not be changed once set, so we set it to `GL_STATIC_DRAW`. */
  free(indices);
  
  Shader::FragDataLocation fs_outs[] = {
    {"FragColor", 0},
  };
  shader.create(R"(
    #version 330 core
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec2 inTexCoord;
    out vec2 TexCoord;
    void main() {
      gl_Position = vec4(inPosition, 0.0, 1.0);
      TexCoord = inTexCoord;
    }
    )"
    , R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D tex0;
    uniform vec3 ColorMask;
    void main() {
      vec4 color = texture(tex0, TexCoord);
      FragColor = vec4(1.0, 1.0, 1.0, color.r) * vec4(ColorMask, 1.0);
    }
    )"
    , sizeof(fs_outs) / sizeof(Shader::FragDataLocation), fs_outs
  );

  this->batch_buf = (uint8_t*)malloc(Font::batch_bufsz);

  fclose(fp);
  return true;
}

void Font::unload() {
  this->font_tex.destroy();
  this->vbuf.destroy();
  this->shader.destroy();
  if (this->batch_buf) {
    free(batch_buf);
    batch_buf = NULL;
  }
  sgl::Font::unload();
}

Font::Font() {
  batch_buf = NULL;
}

Font::~Font() {
  this->unload();
}

void Font::set_line_height(int new_height) {
  sgl::Font::set_line_height(new_height);
}

IVec2 Font::draw_text(const std::wstring & text, int x, int y, const Vec4 & color)
{
  return draw_text(text, x, y, 0, 0, color);
}

IVec2 Font::draw_text(const std::wstring & text, int x, int y, int w, int h, const Vec4 & color)
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  if (text.size() == 0)
    return IVec2(x, y);

  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();
  float g_du = 1.0f / float(rsize.x), g_dv = 1.0f / float(rsize.y);
  float t_du = 1.0f / float(font_tex.get_width()), t_dv = 1.0f / float(font_tex.get_height());

  shader.use();
  shader.set_uniform_3f("ColorMask", float(color.r), float(color.g), float(color.b));
  shader.set_texture_sampler_2D("tex0", font_tex, 0);

  /* render a single line text */
  int x_cursor = 0, y_cursor = 0;
  bool cursor_inited = false;
  uint32_t line_chars = 0; /* number of blitted chars in current line */
  /*
  (x_dst, y_dst) represents the upper-left corner position of the glyph
  when it is about to be blitted onto the target texture.
  */
  int x_dst, y_dst;

  /* Auxiliary function for manipulating cursor position. */
  auto move_cursor_to_new_line = [&](Glyph* glyph) -> bool {
    y_cursor += this->line_height;
    x_cursor = x;
    x_dst = x_cursor + (glyph == NULL ? 0 : glyph->xoffset);
    y_dst = y_cursor - this->line_base + (glyph == NULL ? 0 : glyph->yoffset);
    if (x_dst < 0) {
      x_dst = 0;
      x_cursor = x_dst - (glyph == NULL ? 0 : glyph->xoffset);
    }
    line_chars = 0; /* reset line chars counter */
    /* If a new line exceeds the height limit, we can terminate the whole process. */
    if (y_dst + (glyph == NULL ? 0 : glyph->tex.get_height()) >= y + h)
      return true;
    else return false;
  };

  /* Auxiliary function for flushing (rendering) glyph batch buffer. */
  auto flush_batch = [&](const int count) -> void {
    this->vbuf.subdata_VBO(0, Font::batch_bufsz, this->batch_buf);
    this->vbuf.draw_elements(GL_TRIANGLES, 6 * count, GL_UNSIGNED_INT, NULL);
  };

  /* glyph minibatch buffering */
  int n_out_chars = 0;

  for (size_t i = 0; i < text.size(); i++) {
    /*
    When encountering a newline character ('\n'), start a new
    line immediately.
    */
    if ((uint32_t)text[i] == (uint32_t)'\n') {
      move_cursor_to_new_line(NULL);
      continue;
    }
    /*
    Load glyph.
    */
    if (this->charmap.find((uint32_t)text[i]) == this->charmap.end())
      continue;
    Glyph& glyph = this->charmap[(uint32_t)text[i]];
    if (!cursor_inited) {
      x_cursor = x;
      y_cursor = y + this->line_base;
      cursor_inited = true;
    }
    /*
    Calculate the default blit destination position.
    Note the following adjustments:
      * If this is the first character of the current line and x_dst
      is less than zero (which can happen because glyph.xoffset may
      sometimes be negative), ensure x_dst is non-negative.
      * If this is not the first character of the current line, the
      kerning between the current and previous character must be
      considered.
    */
    std::pair<uint32_t, uint32_t> kerning_pair;
    if (i > 0)
      kerning_pair = std::make_pair((uint32_t)text[i], (uint32_t)text[i - 1]);
    if (line_chars > 0 && this->kernings.find(kerning_pair) != this->kernings.end()) {
      int32_t kerning_amount = this->kernings[kerning_pair];
      x_cursor += kerning_amount;
    }
    x_dst = x_cursor + glyph.xoffset;
    y_dst = y_cursor - this->line_base + glyph.yoffset;
    /*
    Check if the current glyph is outside the text box. If so,
    a new line must be started. However, if the text box width
    is too small, the glyph must be displayed regardless.
    Note:
      * If w is less than or equal to 0, the text box region is
      ignored, and the entire text will be displayed on a single
      line.
    */
    bool requires_new_line;
    if (w <= 0 || line_chars == 0 || glyph.is_empty)
      requires_new_line = false;
    else if (x_dst + glyph.tex.get_width() > x + w)
      requires_new_line = true;
    else
      requires_new_line = false;
    if (requires_new_line) {
      bool cursor_exceeds_height_limit = move_cursor_to_new_line(&glyph);
      if (cursor_exceeds_height_limit) {
        /* 
        Exit early as the text exceeds the boundaries of the text box, 
        but don't forget to flush remained chars.
        */
        flush_batch(n_out_chars % Font::batch_size);
        return IVec2(x_cursor, y_cursor);
      }
    }
    /*
    Render glyph to texture.
    NOTE: we pack multiple glyphs to a minibatch to improve render speed.
    */
    {
      float g_xl = 2.0f * float(x_dst) * g_du - 1.0f;
      float g_xr = 2.0f * float(x_dst + glyph.w) * g_du - 1.0f;
      float g_yb = 1.0f - 2.0f * float(y_dst + glyph.h) * g_dv;
      float g_yt = 1.0f - 2.0f * float(y_dst) * g_dv;

      float t_xl = t_du * float(glyph.x);
      float t_xr = t_du * float(glyph.x + glyph.w);
      float t_yb = t_dv * float(font_tex.get_height() - glyph.y - glyph.h);
      float t_yt = t_dv * float(font_tex.get_height() - glyph.y);

      float glyph_vbuf[16] = {
        /*
        We directly compute normalized device coordinates (NDC) here. The vertex 
        shader performs minimal processing - it simply passes through the input 
        attributes and lets them interpolate naturally to the fragment shader.
        */
        g_xl, g_yb, t_xl, t_yb,
        g_xr, g_yb, t_xr, t_yb,
        g_xr, g_yt, t_xr, t_yt,
        g_xl, g_yt, t_xl, t_yt,
      };

      memcpy(this->batch_buf + sizeof(glyph_vbuf) * (n_out_chars % Font::batch_size), glyph_vbuf, sizeof(glyph_vbuf));
      n_out_chars++;

      /* flush if batch is full */
      if (n_out_chars % Font::batch_size == 0)
        flush_batch(Font::batch_size);
    }
    line_chars++;
    x_cursor += glyph.xadvance;
  }

  /* flush remained chars */
  flush_batch(n_out_chars % Font::batch_size);

  return IVec2(x_cursor, y_cursor);
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

SpriteRenderer::SpriteRenderer() {}

SpriteRenderer::~SpriteRenderer() {
  this->destroy();
}

void SpriteRenderer::draw(
  sgl::OpenGL::Texture* source, int target_w, int target_h,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const sgl::SpriteOriginMode origin_mode,
  const sgl::OpenGL::Shader* custom_shader)
{
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);

  if (source->get_device() != DeviceType_GPU) {
    printf("Error, texture is not transferred to GPU.\n");
    return;
  }

  float du = 1.0f / float(source->get_width()), dv = 1.0f / float(source->get_height());
  float lx = du * float(src_x);
  float rx = du * float(src_x + src_w);
  float by = dv * float(source->get_height() - src_y - src_h);
  float ty = dv * float(source->get_height() - src_y);

  const sgl::OpenGL::Shader* shader = &this->shader;
  if (custom_shader != NULL)
    shader = custom_shader;

  shader->use();
  shader->set_uniform_3f("ColorMask", float(color_mask.r), float(color_mask.g), float(color_mask.b));
  shader->set_texture_sampler_2D("tex0", *source, 0);
  shader->set_uniform_4f("Transform", float(scale.x), float(scale.y), float(dst_x), float(target_h - dst_y)); /* Note the "target_h - dst_y" adjustment, as the Y-axis in screen space differs from the one defined in OpenGL's NDC (Normalized Device Coordinates). */
  shader->set_uniform_4i("TexDims", src_w, src_h, target_w, target_h);
  shader->set_uniform_1f("Rotation", float(rot));

  if (origin_mode == SpriteOriginMode_Center) {
    float vertices[16] = {
      -0.5f, -0.5f, lx, by,
      +0.5f, -0.5f, rx, by,
      +0.5f, +0.5f, rx, ty,
      -0.5f, +0.5f, lx, ty,
    };
    this->vbuf.subdata_VBO(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_BottomLeft) {
    float vertices[16] = {
      +0.0f, +0.0f, lx, by,
      +1.0f, +0.0f, rx, by,
      +1.0f, +1.0f, rx, ty,
      +0.0f, +1.0f, lx, ty,
    };
    this->vbuf.subdata_VBO(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_BottomRight) {
    float vertices[16] = {
      -1.0f, +0.0f, lx, by,
      +0.0f, +0.0f, rx, by,
      +0.0f, +1.0f, rx, ty,
      -1.0f, +1.0f, lx, ty,
    };
    this->vbuf.subdata_VBO(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_TopLeft) {
    float vertices[16] = {
      +0.0f, -1.0f, lx, by,
      +1.0f, -1.0f, rx, by,
      +1.0f, +0.0f, rx, ty,
      +0.0f, +0.0f, lx, ty,
    };
    this->vbuf.subdata_VBO(0, sizeof(vertices), vertices);
  }
  else if (origin_mode == SpriteOriginMode_TopRight) {
    float vertices[16] = {
      -1.0f, -1.0f, lx, by,
      +0.0f, -1.0f, rx, by,
      +0.0f, +0.0f, rx, ty,
      -1.0f, +0.0f, lx, ty,
    };
    this->vbuf.subdata_VBO(0, sizeof(vertices), vertices);
  }
  this->vbuf.draw_elements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
}

void SpriteRenderer::initialize(const std::string & vs, const std::string & fs, const int n_outs, const Shader::FragDataLocation * fs_outs)
{
  std::string _vs = (strlen(vs.c_str()) == 0) ? R"(
    #version 330 core
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec2 inTexCoord;
    uniform ivec4 TexDims;  /* (Sw, Sh, Tw, Th) */
    uniform vec4 Transform; /* (Sx, Sy, dx, dy) */
    uniform float Rotation;
    out vec2 TexCoord;
    mat4x4 rotate_z(float angle) {
      float c = cos(angle), s = sin(angle);
      return mat4x4(c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
    }
    mat4x4 translate(float dx, float dy, float dz) {
      return mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, dx, dy, dz, 1);
    }
    mat4x4 scale(float sx, float sy, float sz) {
      return mat4x4(sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, sz, 0, 0, 0, 0, 1);
    }
    mat4x4 ortho(float n, float f, float l, float r, float t, float b) {
      return mat4x4(2/(r-l), 0, 0, 0, 0, 2/(t-b), 0, 0, 0, 0, -2/(f-n), 0, 
        -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1);
    }
    void main() {
      mat4x4 mTranslate = translate(Transform.z, Transform.w, 0);
      mat4x4 mRotate = rotate_z(Rotation);
      mat4x4 mScale = scale(Transform.x, Transform.y, 1);
      mat4x4 mModel = mTranslate * mRotate * mScale;
      mat4x4 mProjection = ortho(0, 1, 0, TexDims.z, TexDims.w, 0);
      mat4x4 mTransform = mProjection * mModel;
      gl_Position = mTransform * vec4(TexDims.xy * inPosition.xy, 0, 1);
      TexCoord = inTexCoord;
    }
    )" : vs;
  std::string _fs = (strlen(fs.c_str()) == 0) ? R"(
    #version 330 core
    layout(location = 0) out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D tex0;
    uniform vec3 ColorMask;
    void main() {
      vec4 color = texture(tex0, TexCoord); 
      FragColor = color * vec4(ColorMask, 1);
    }
    )" : fs;
  Shader::FragDataLocation _fs_outs_default[] = {
      {"FragColor", 0},
  };
  const Shader::FragDataLocation* _fs_outs = (fs_outs == NULL) ? _fs_outs_default : fs_outs;
  int _n_outs = (fs_outs == NULL) ? sizeof(_fs_outs_default) / sizeof(Shader::FragDataLocation) : n_outs;
  shader.create(_vs, _fs, _n_outs, _fs_outs);  
  int indices[] = { 0, 1, 3, 1, 2, 3 };
  vbuf.create_and_fill(16 * sizeof(float), NULL, GL_DYNAMIC_DRAW, 6 * sizeof(indices), indices, GL_STATIC_DRAW); /* Index buffer will not be changed once set, so we set it to `GL_STATIC_DRAW`. */
}

void blit_texture(sgl::OpenGL::Texture* source, int target_w, int target_h,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const sgl::SpriteOriginMode origin_mode,
  const sgl::OpenGL::Shader* custom_shader)
{
  PixelFormat format = source->get_pixel_format();
  if (format == PixelFormat_BGRA8888 || format == PixelFormat_RGBA8888)
    gl_vars.sprite_renderer_RGBA.draw(source, target_w, target_h, src_x, src_y, src_w, src_h, dst_x, dst_y, scale, rot, color_mask, origin_mode, custom_shader);
  else if (format == PixelFormat_Float32)
    gl_vars.sprite_renderer_R32F.draw(source, target_w, target_h, src_x, src_y, src_w, src_h, dst_x, dst_y, scale, rot, color_mask, origin_mode, custom_shader);
  else if (format == PixelFormat_OpenGL_RG32F)
    gl_vars.sprite_renderer_RG32F.draw(source, target_w, target_h, src_x, src_y, src_w, src_h, dst_x, dst_y, scale, rot, color_mask, origin_mode, custom_shader);
  else {
    printf("Unsupported pixel format.\n");
    return;
  }
}

void FrameBuffer::setup_attachment(sgl::OpenGL::Texture * tex, int slot) {
  if (slot < 0 || slot >= 8 || slot >= gl_vars.max_color_attachments) {
    printf("Error, invalid slot id.\n");
    return;
  }
  if (tex->get_device() != DeviceType_GPU) {
    printf("Error, texture not in GPU.\n");
    return;
  }
  this->color_slots[slot] = tex;
}

bool FrameBuffer::make() {
  if (fbo != 0) {
    printf("Error, framebuffer is already initialized, call destroy() before make().\n");
    return false;
  }

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  GLenum draw_buffers[8];

  int w = -1, h = -1;
  int num_draw_buffers = 0;
  for (int i = 0; i < 8; i++) {
    if (i >= gl_vars.max_color_attachments) break;
    if (color_slots[i] == NULL) continue;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, color_slots[i]->get_GL_handle(), 0);
    if (w < 0 || h < 0) {
      w = color_slots[i]->get_width();
      h = color_slots[i]->get_height();
    }
    else {
      if (w != color_slots[i]->get_width() || h != color_slots[i]->get_height()) {
        printf("Error: Color texture size mismatch detected.\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
      }
    }
    draw_buffers[num_draw_buffers] = GL_COLOR_ATTACHMENT0 + i;
    num_draw_buffers++;
  }
  this->w = w;
  this->h = h;

  glDrawBuffers(num_draw_buffers, draw_buffers);

  /* generate a depth stencil texture and attach it to framebuffer */
  glGenTextures(1, &depth_stencil_texid);
  glBindTexture(GL_TEXTURE_2D, depth_stencil_texid);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, w, h, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depth_stencil_texid, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  /* check framebuffer completeness and return */
  GLenum fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Framebuffer not complete! Error code = %d\n", fbo_status);
  }

  /* compile shader for blit framebuffer */
  blit_shader.create(R"(
    #version 330 core
    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec2 inTexCoords;
    out vec2 TexCoords;
    void main()
    {
      gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
      TexCoords = inTexCoords;
    }
    )", R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoords;
    uniform sampler2D tex0;
    void main()
    {
      FragColor = texture(tex0, TexCoords);
    }
    )"
  );
  float quad_verts[] = { 
    /* vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates. */
    /* positions   texCoords */
    -1.0f, +1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
    +1.0f, -1.0f,  1.0f, 0.0f,
    -1.0f, +1.0f,  0.0f, 1.0f,
    +1.0f, -1.0f,  1.0f, 0.0f,
    +1.0f, +1.0f,  1.0f, 1.0f,
  };
  quad_vbuf.create_and_fill(sizeof(quad_verts), quad_verts, GL_STATIC_DRAW, 0, NULL, GL_STATIC_DRAW);

  return fbo_status == GL_FRAMEBUFFER_COMPLETE && blit_shader.get_GL_handle() != 0 && quad_vbuf.get_VAO_GL_handle() != 0;
}

void FrameBuffer::destroy() {
  if (fbo != 0) {
    glDeleteFramebuffers(1, &fbo);
    fbo = 0;
  }
  if (depth_stencil_texid != 0) {
    glDeleteTextures(1, &depth_stencil_texid);
    depth_stencil_texid = 0;
  }
  for (int i = 0; i < 8; i++)
    color_slots[i] = NULL;
  blit_shader.destroy();
  quad_vbuf.destroy();
  w = h = -1;
}

int FrameBuffer::get_width() const { 
  return this->w; 
}

int FrameBuffer::get_height() const { 
  return this->h; 
}

void FrameBuffer::bind() {
  if (fbo == 0) {
    printf("Error, cannot bind framebuffer since it is invalid/incomplete.\n");
    return;
  }
  GLuint current_fbo = sgl::OpenGL::get_current_framebuffer();
  if (current_fbo == fbo) {
    printf("Error, cannot bind the same framebuffer twice.\n");
    return;
  }
  else if (current_fbo != 0) {
    printf("Error, cannot bind framebuffer since some other framebuffer is already bound.\n");
    return;
  }

  IVec2 rsize = sgl::OpenGL::get_OpenGL_framebuffer_size(fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glViewport(0, 0, rsize.x, rsize.y);
}

void FrameBuffer::unbind() {
  if (fbo == 0) {
    printf("Error, cannot unbind framebuffer since it is invalid/incomplete.\n");
    return;
  }
  GLuint current_fbo = sgl::OpenGL::get_current_framebuffer();
  if (current_fbo != fbo) {
    printf("Error, cannot unbind framebuffer, internal handle mismatch.\n");
    return;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  IVec2 rsize = sgl::OpenGL::get_OpenGL_framebuffer_size(0);
  glViewport(0, 0, rsize.x, rsize.y);
}

GLuint FrameBuffer::get_GL_handle() const { 
  return fbo;
}

void FrameBuffer::blit_attachment_to_main_framebuffer(int slot, int dst_x, int dst_y, int dst_w, int dst_h) {

  IVec2 size = sgl::OpenGL::get_OpenGL_framebuffer_size(0);

  glBindFramebuffer(GL_FRAMEBUFFER, 0); /* select main framebuffer */
  glDisable(GL_DEPTH_TEST);

  blit_shader.use();
  blit_shader.set_texture_sampler_2D("tex0", *color_slots[slot], 0);
  float iW = 1.0f / float(size.x), iH = 1.0f / float(size.y);
  float xl = 2 * dst_x * iW - 1.0f, xr = 2 * (dst_x + dst_w) * iW - 1.0f;
  float yt = 1.0f - 2 * dst_y * iH, yb = 1.0f - 2 * (dst_y + dst_h) * iH;
  float quad_verts[] = {
    /* positions   texCoords */
    xl, yb, 0.0f, 0.0f,
    xr, yb, 1.0f, 0.0f,
    xl, yt, 0.0f, 1.0f,
    xl, yt, 0.0f, 1.0f,
    xr, yb, 1.0f, 0.0f,
    xr, yt, 1.0f, 1.0f,
  };
  quad_vbuf.subdata_VBO(0, sizeof(quad_verts), quad_verts);
  quad_vbuf.draw_arrays(GL_TRIANGLES, 0, 6);
}

sgl::OpenGL::Texture FrameBuffer::extract_depth_buffer()
{
  sgl::OpenGL::Texture tex = sgl::create_texture(w, h, PixelFormat_Float32, TextureSampling_Nearest, TextureUsage_DepthBuffer);

  this->bind();
  {
    float* pixel_ptr = (float*)tex.get_pixel_data();
    glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, pixel_ptr);
  }
  this->unbind();
  
  return tex;
}

FrameBuffer::FrameBuffer() {
  for (int i = 0; i < 8; i++)
    color_slots[i] = NULL;
  fbo = 0;
  depth_stencil_texid = 0;
  w = h = -1;
}

FrameBuffer::~FrameBuffer() {
  destroy();
}

AnimatedModelRenderer::AnimatedModelRenderer()
{
  this->play_time = 0.0;
}

AnimatedModelRenderer::~AnimatedModelRenderer()
{
  unload();
}

void AnimatedModelRenderer::play_animation(const std::string& anim_name, const double& play_time)
{
  this->anim_name = anim_name; this->play_time = play_time;
}

bool AnimatedModelRenderer::load_model_zip(const std::string& zip_file, const std::string& model_fname)
{
  this->unload();

  /* load model to CPU host memory */
  if (!model.load_zip(zip_file, model_fname))
    return false;

  /* transfer model from CPU host memory to GPU VRAM */
  const std::vector<Mesh>& mesh_data = model.get_meshes();
  const std::vector<Material>& materials = model.get_materials();
  auto _transform_vertex = [](const Vertex_pnt_nm_bone& vert) -> Vertex_t {
    Vertex_t vert_new;
    vert_new.position[0] = float(vert.position.x); vert_new.normal[0] = float(vert.normal.x);
    vert_new.position[1] = float(vert.position.y); vert_new.normal[1] = float(vert.normal.y);
    vert_new.position[2] = float(vert.position.z); vert_new.normal[2] = float(vert.normal.z);
    vert_new.texcoord[0] = float(vert.texcoord.x);
    vert_new.texcoord[1] = float(vert.texcoord.y);
    vert_new.tangent[0] = float(vert.tangent.x); vert_new.bitangent[0] = float(vert.bitangent.x);
    vert_new.tangent[1] = float(vert.tangent.y); vert_new.bitangent[1] = float(vert.bitangent.y);
    vert_new.tangent[2] = float(vert.tangent.z); vert_new.bitangent[2] = float(vert.bitangent.z);
    vert_new.bone_IDs[0] = vert.bone_IDs.i[0];
    vert_new.bone_IDs[1] = vert.bone_IDs.i[1];
    vert_new.bone_IDs[2] = vert.bone_IDs.i[2];
    vert_new.bone_IDs[3] = vert.bone_IDs.i[3];
    vert_new.bone_weights[0] = float(vert.bone_weights.i[0]);
    vert_new.bone_weights[1] = float(vert.bone_weights.i[1]);
    vert_new.bone_weights[2] = float(vert.bone_weights.i[2]);
    vert_new.bone_weights[3] = float(vert.bone_weights.i[3]);
    return vert_new;
  };
  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    const int32_t mat_id = mesh_data[i_mesh].mat_id;
    const std::vector<Vertex_pnt_nm_bone>& vertices = mesh_data[i_mesh].vertices;
    const std::vector<int32_t>& indices = mesh_data[i_mesh].indices;
    /* convert vertex format (float64 to float32) */
    std::vector<Vertex_t> vbuf_cpu(vertices.size());
    for (uint32_t i_vert = 0; i_vert < vertices.size(); i_vert++)
      vbuf_cpu[i_vert] = _transform_vertex(vertices[i_vert]);
    /* load vertices and indices into GPU VRAM */
    VertexBuffer_t* vbuf_gpu = new VertexBuffer_t();
    vbuf_gpu->create_and_fill(int(vertices.size() * sizeof(Vertex_t)), vbuf_cpu.data(), GL_STATIC_DRAW, int(sizeof(int) * indices.size()), indices.data(), GL_STATIC_DRAW);
    this->vbufs.push_back(vbuf_gpu);
    /* load and transfer textures */
    void* tex_cpu_dptr = materials[mat_id].diffuse_texture.get_pixel_data();
    if (tex_cpu_dptr != NULL && this->texmap.find(tex_cpu_dptr) == this->texmap.end()) {
      /* this is a new texture, reigster and transfer it to GPU */
      sgl::OpenGL::Texture* tex_gpu = new sgl::OpenGL::Texture();
      *tex_gpu = materials[mat_id].diffuse_texture;
      tex_gpu->to_device(DeviceType_GPU);
      this->texmap.insert_or_assign(tex_cpu_dptr, tex_gpu);
    }
  }

  /* create shader */
  typedef sgl::OpenGL::Shader::FragDataLocation FragDataLocation;
  FragDataLocation fs_outs[] = {
    {"FragColor", 0},
    {"FragNormal", 1},
  };
  if (!this->shader.create(
    sgl::read_file_as_string("assets/common/shaders/test_opengl_animated/anim.vert"),
    sgl::read_file_as_string("assets/common/shaders/test_opengl_animated/anim.frag"),
    sizeof(fs_outs) / sizeof(FragDataLocation), fs_outs))
  {
    this->unload();
    return false;
  }

  return true;
}

void AnimatedModelRenderer::draw()
{
  const IVec2 rsize = sgl::OpenGL::get_current_render_target_size();

  Mat4x4 view = this->get_view_matrix();
  Mat4x4 proj = this->get_projection_matrix(rsize.x, rsize.y);
  Mat4x4 bone_matrices[MAX_NODES_PER_MODEL];

  this->shader.use();
  this->shader.set_uniform_matrix_4fv("u_Model", 1, GL_TRUE, &this->model.get_model_transform());
  this->shader.set_uniform_matrix_4fv("u_View", 1, GL_TRUE, &view);
  this->shader.set_uniform_matrix_4fv("u_Projection", 1, GL_TRUE, &proj);

  /* Rendering all the mesh parts in model */
  const std::vector<Mesh>& mesh_data = this->model.get_meshes();
  const std::vector<Material>& materials = this->model.get_materials();

  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    /* for each mesh part */
    const int32_t mat_id = mesh_data[i_mesh].mat_id;
    const Mesh& mesh = mesh_data[i_mesh];
    /* update bone matrices */
    this->model.update_skeletal_animation_for_mesh(mesh, anim_name, play_time, bone_matrices);
    this->shader.set_uniform_matrix_4fv("u_BoneMatrices", MAX_NODES_PER_MODEL, GL_TRUE, bone_matrices);
    /* setup texture(s) */
    void* tex_cpu_dptr = materials[mat_id].diffuse_texture.get_pixel_data();
    if (this->texmap.find(tex_cpu_dptr) != this->texmap.end()) {
      sgl::OpenGL::Texture* tex_gpu = this->texmap[tex_cpu_dptr];
      this->shader.set_texture_sampler_2D("tex0", *tex_gpu, 0);
    }
    /* draw */
    this->vbufs[i_mesh]->draw_elements(GL_TRIANGLES, int(mesh_data[i_mesh].indices.size()), GL_UNSIGNED_INT, NULL);
  }
}

void AnimatedModelRenderer::unload()
{
  this->model.unload();
  this->shader.destroy();

  for (int i = 0; i < vbufs.size(); i++) {
    this->vbufs[i]->destroy();
    delete this->vbufs[i];
  }
  this->vbufs.clear();
  this->vbufs.shrink_to_fit();

  for (std::map<void*, sgl::OpenGL::Texture*>::iterator it = this->texmap.begin();
    it != this->texmap.end(); it++)
  {
    sgl::OpenGL::Texture* tex_gpu = it->second;
    delete tex_gpu;
  }
  this->texmap.clear();

  this->anim_name = "";
  this->play_time = 0.0;
}

}; /* namespace OpenGL */
}; /* namespace sgl */

#endif
