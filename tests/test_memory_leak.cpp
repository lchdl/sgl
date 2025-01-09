#include <stdio.h>
#include "sgl.h"

using namespace sgl;

void test_texture_memory_leak() {
  const int total_loops = 50000;
  for (int loop = 0; loop < total_loops; loop++) {
    printf("Loop [%d/%d].\n", loop + 1, total_loops);
    Texture tex0 = sgl::create_texture(512, 512, PixelFormat_RGBA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
    Texture tex1 = sgl::load_texture("assets/tests/textures/checker_256.png", PixelFormat_BGRA8888);
    Texture tex2 = tex1.to_format(PixelFormat_RGBA8888);
    Texture tex3 = tex2;
    tex0 = tex1;
    tex2 = tex3;
    tex3 = tex0;
  }
}

/**

NOTE: Run memory leak tests in DEBUG mode, 
as RELEASE mode may optimize many loops out.

**/

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */
  test_texture_memory_leak();
  return 0;
}
