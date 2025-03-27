#include <stdio.h>
#include "sgl.h"

using namespace sgl;

void test_font_memory_leak() {
  const int total_loops = 1000;
  for (int loop = 0; loop < total_loops; loop++) {
    printf("test_font_memory_leak >> Loop [%d/%d].\r", loop + 1, total_loops);
    sgl::Font Arial_11pt, Arial_11pt_copy;
    Arial_11pt.load("assets/common/fonts/Arial/11pt_Regular.fnt");
    Arial_11pt_copy = Arial_11pt;
    Arial_11pt = Arial_11pt_copy;
  }
  printf("test_font_memory_leak FINISHED.\n");
}

void test_texture_memory_leak() {
  const int total_loops = 1000;
  for (int loop = 0; loop < total_loops; loop++) {
    printf("test_texture_memory_leak >> Loop [%d/%d].\r", loop + 1, total_loops);
    Texture tex0 = sgl::create_texture(512, 512, PixelFormat_RGBA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
    Texture tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
    Texture tex2 = tex1.to_format(PixelFormat_RGBA8888);
    Texture tex3 = tex2;
    tex0 = tex1;
    tex2 = tex3;
    tex3 = tex0;
    
  }
  printf("test_texture_memory_leak FINISHED.\n");
}

#ifdef ENABLE_OPENGL
void test_OpenGL_texture() {
  sgl::Texture tex = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
  sgl::OpenGL::Texture gl_tex1 = sgl::OpenGL::Texture(tex);
  sgl::OpenGL::Texture gl_tex2;

  const int total_loops = 100000;
  for (int loop = 0; loop < total_loops; loop++) {
    printf("test_OpenGL_texture >> Loop [%d/%d].\r", loop + 1, total_loops);
    sgl::OpenGL::Texture gl_tex3;
    gl_tex2 = gl_tex1;
    gl_tex1.destroy();
    gl_tex1 = gl_tex2;
    gl_tex3 = gl_tex1;
    gl_tex2 = gl_tex3;
    gl_tex1.destroy();
    gl_tex1 = gl_tex3;
    gl_tex3.to(sgl::DeviceType_GPU);
    printf("%d (<-should always be 1)", gl_tex3.get_GL_handle());

    sgl::OpenGL::Texture tex = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
    std::map<int, sgl::OpenGL::Texture> map_int2tex;
    map_int2tex.insert_or_assign(0, tex);
    tex.to(DeviceType_GPU);
    tex.to(DeviceType_CPU);
    tex.to(DeviceType_GPU);
    tex.to(DeviceType_CPU);
  }
  printf("\n");
  printf("test_OpenGL_texture FINISHED.\n");
}
#endif

/**

NOTE: Run memory leak tests in DEBUG mode, 
as RELEASE mode may optimize many loops out.

**/

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */
  printf("Simple memory leak CI tests.\n");
  printf("Run these tests in DEBUG mode.\n");

#ifdef ENABLE_OPENGL
  SDL_Window* pWindow = SDL_CreateWindow("Dummy Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  if (pWindow == NULL) exit(1);
  if (!sgl::OpenGL::initialize_OpenGL(pWindow, 3, 3, true)) exit(1);
  test_OpenGL_texture();
#endif

  test_font_memory_leak();
  test_texture_memory_leak();

  return 0;
}
