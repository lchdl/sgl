
#ifdef WINDOWS
/**

NOTE: Run memory leak tests in DEBUG mode,
as RELEASE mode may optimize many loops out.

Memory leak detection sample:
https://github.com/microsoft/VCSamples/blob/master/VC2010Samples/crt/crt_dbg1/crt_dbg1.c

**/

#include <Windows.h>
#include <crtdbg.h> /* for detecting memory leaks */
#include <stdio.h>
#include "sgl.h"
using namespace sgl;

typedef void (*memory_leak_test_func_t)(void);

struct memchk_states {
  static const int bufsize = 4096;
  char fail_reason[memchk_states::bufsize];
} _states;
void SetMemLeakChkFailed(const std::string& reason) {
  memset(_states.fail_reason, 0, memchk_states::bufsize);
  memcpy_s(_states.fail_reason, memchk_states::bufsize - 1, reason.c_str(), reason.size());
}
enum ConsoleTextColor {
  BLACK = 0, BLUE = 1, GREEN = 2, CYAN = 3,
  RED = 4, MAGENTA = 5, YELLOW = 6, WHITE = 7,
  GRAY = 8, BRIGHT_BLUE = 9, BRIGHT_GREEN = 10, BRIGHT_CYAN = 11,
  BRIGHT_RED = 12, BRIGHT_MAGENTA = 13, BRIGHT_YELLOW = 14, BRIGHT_WHITE = 15
};
void SetColor(ConsoleTextColor color) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  SetConsoleTextAttribute(hConsole, color);
}
void CrtMemChkSetup() {
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
}
bool RunMemLeakTest(memory_leak_test_func_t func, std::string test_name, const int n_repeat = 1) {
  SetColor(WHITE);
  printf("%s >> testing...", test_name.c_str());
  _CrtMemState s0, s1, s2; /* s0 = s2 - s1 */
  _CrtMemCheckpoint(&s1);
  func();
  _CrtMemCheckpoint(&s2);
  if (_CrtMemDifference(&s0, &s1, &s2) || _states.fail_reason[0] != '\0') {
    SetColor(BRIGHT_RED);
    printf("  FAILED.\n");
    if (_states.fail_reason[0] != '\0') {
      printf("Returned user error message:\n");
      SetColor(BRIGHT_YELLOW);
      printf("%s\n", _states.fail_reason);
      memset(_states.fail_reason, 0, memchk_states::bufsize);
    }
    SetColor(BRIGHT_RED);
    printf("Dumping memory leak info...\n");
    SetColor(BRIGHT_RED);
    printf("_CrtMemDumpStatistics():\n");
    SetColor(BRIGHT_WHITE);
    _CrtMemDumpStatistics(&s0);
    SetColor(BRIGHT_RED);
    printf("_CrtDumpMemoryLeaks():\n");
    SetColor(BRIGHT_WHITE);
    _CrtDumpMemoryLeaks();
    SetColor(WHITE);
    return false;
  }
  else {
    SetColor(BRIGHT_GREEN);
    printf("  PASSED.\n");
    SetColor(BRIGHT_WHITE);
    return true;
  }
  return false;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void test_func_template() {
  /* 

  Do tests here...
  If you want to force test to fail, do this

  >>> SetMemLeakChkFailed("write fail reason here.");

  */
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void test_model_copy() {
  Model model;
  model.load_zip("assets/common/models/boblamp.zip", "model.md5mesh");
  Model model0 = model;
  Model model1(model);
  Model model2;
  model2.load_zip("assets/common/models/boblamp.zip", "model.md5mesh");
  model2 = model;
  model = model2;
  model1 = model2;
  model2 = model1;
}
void test_font_memory_leak() {
  sgl::Font Arial_11pt, Arial_11pt_copy;
  Arial_11pt.load("assets/common/fonts/Arial/11pt_Regular.fnt");
  Arial_11pt_copy = Arial_11pt;
  Arial_11pt = Arial_11pt_copy;
}
void test_texture_memory_leak() {
  Texture tex0 = sgl::create_texture(512, 512, PixelFormat_RGBA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  Texture tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
  Texture tex2 = tex1.to_format(PixelFormat_RGBA8888);
  Texture tex3 = tex2;
  tex0 = tex1;
  tex2 = tex3;
  tex3 = tex0;
}
#ifdef ENABLE_OPENGL
void test_OpenGL_texture() {
  sgl::Texture tex = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
  sgl::OpenGL::Texture gl_tex1 = sgl::OpenGL::Texture(tex);
  sgl::OpenGL::Texture gl_tex2;

  const int total_loops = 100;
  GLint tex_handle = -1;
  for (int loop = 0; loop < total_loops; loop++) {
    sgl::OpenGL::Texture gl_tex3;
    gl_tex2 = gl_tex1;
    gl_tex1.destroy();
    gl_tex1 = gl_tex2;
    gl_tex3 = gl_tex1;
    gl_tex2 = gl_tex3;
    gl_tex1.destroy();
    gl_tex1 = gl_tex3;
    gl_tex3.to_device(sgl::DeviceType_GPU);
    tex_handle = gl_tex3.get_GL_handle();

    sgl::OpenGL::Texture tex = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
    std::map<int, sgl::OpenGL::Texture> map_int2tex;
    map_int2tex.insert_or_assign(0, tex);
    tex.to_device(DeviceType_GPU);
    tex.to_device(DeviceType_CPU);
    tex.to_device(DeviceType_GPU);
    tex.to_device(DeviceType_CPU);

    sgl::OpenGL::Texture q = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
    q.to_device(DeviceType_GPU);
    q = q.to_format(PixelFormat_RGBA8888);
    q = q.to_format(PixelFormat_RGBA8888);
  }
  if (tex_handle != 1) {
    SetMemLeakChkFailed("OpenGL texture handle leaked!");
  }
}
void test_OpenGL_texture_v2() {
  const int total_loops = 100;
  GLint tex_handle = -1;
  for (int loop = 0; loop < total_loops; loop++) {
    sgl::OpenGL::Texture gl_tex1 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
    gl_tex1.to_device(DeviceType_GPU);
    tex_handle = gl_tex1.get_GL_handle();
    sgl::OpenGL::Texture gl_tex2 = gl_tex1;
    sgl::OpenGL::Texture gl_tex3;
    gl_tex3 = gl_tex1;
    gl_tex1 = gl_tex2;
    gl_tex1 = gl_tex3;
    gl_tex2 = gl_tex3;
    gl_tex3 = gl_tex2;
  }
  if (tex_handle != 1) {
    SetMemLeakChkFailed("OpenGL texture handle leaked!");
  }
}
#endif

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */
  printf("Memory leak tests.\n");
  printf("** Run these tests in DEBUG mode. **\n");
  CrtMemChkSetup();

  /* Run memory leak tests here. */

  RunMemLeakTest(test_model_copy, "test_model_copy");
  RunMemLeakTest(test_font_memory_leak, "test_font_memory_leak");
  RunMemLeakTest(test_texture_memory_leak, "test_texture_memory_leak");

#ifdef ENABLE_OPENGL
  SDL_Window* pWindow = SDL_CreateWindow("Dummy Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64, 64, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL);
  if (pWindow == NULL) exit(1);
  if (!sgl::OpenGL::initialize_OpenGL(pWindow, 3, 3, true)) exit(1);
  RunMemLeakTest(test_OpenGL_texture,"test_OpenGL_texture");
  RunMemLeakTest(test_OpenGL_texture_v2, "test_OpenGL_texture_v2");
#endif

  /* TODO: add more memory leak tests here... */

  return 0;
}

#else
int main() {
  printf("Compile on Windows to test memory leak.\n");
}
#endif
