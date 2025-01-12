#include <stdio.h>
#include "sgl.h"

using namespace sgl;

void test_font_memory_leak() {
  const int total_loops = 1000;
  for (int loop = 0; loop < total_loops; loop++) {
    printf("test_font_memory_leak >> Loop [%d/%d].\r", loop + 1, total_loops);
    sgl::Font Arial_11pt, Arial_11pt_copy;
    Arial_11pt.load("assets/standard/fonts/Arial/11pt_Regular.fnt");
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
    Texture tex1 = sgl::load_texture("assets/standard/textures/checker_256.png", PixelFormat_BGRA8888);
    Texture tex2 = tex1.to_format(PixelFormat_RGBA8888);
    Texture tex3 = tex2;
    tex0 = tex1;
    tex2 = tex3;
    tex3 = tex0;
  }
  printf("test_texture_memory_leak FINISHED.\n");
}

/**

NOTE: Run memory leak tests in DEBUG mode, 
as RELEASE mode may optimize many loops out.

**/

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */
  printf("Simple memory leak CI tests.\n");
  printf("Run these tests in DEBUG mode.\n");

  test_font_memory_leak();
  test_texture_memory_leak();

  return 0;
}
