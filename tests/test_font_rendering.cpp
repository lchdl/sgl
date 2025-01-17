#include <stdio.h>
#include <sstream>
#include "sgl.h"

using namespace sgl;

bool keystate[SDL_NUM_SCANCODES];
SDL_Window* pWindow;
SDL_Surface* pWindowSurface;

const int w = 640, h = 480;

sgl::Texture tex_640x480;
sgl::Texture tex_320x240;

sgl::Font Arial_11pt;
sgl::Font MiniHerz_16pt;
sgl::Font Beatixel_16pt;
sgl::Font CuteBlockhead_16pt;
sgl::Font GrapeSoda_16pt;
sgl::Font HP_100LX_6x8_8pt;
sgl::Font IBM_CGA_8x8_8pt;
sgl::Font IBM_CGAthin_8x8_8pt;
sgl::Font IBM_EGA_8x14_16pt;
sgl::Font Superscript_16pt;
sgl::Font Boutique_7pt;
sgl::Font Boutique_9pt;
sgl::Font Vonwaon_12pt;
std::wstring text_intro;
std::wstring text_os437;
std::wstring text_chinese;

int demo_page = 1;
const int total_demos = 8;
double T_frame = 0.0, T_global = 0.0;
int frameid = 0;

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
  if (is_press && (keycode == SDLK_LEFT || keycode == SDLK_RIGHT)) {
    tex_640x480.clear(Vec4(0, 0, 0, 1));
    T_frame = 0.0;
    frameid = 0;
    if (keycode == SDLK_LEFT) {
      demo_page--;
      if (demo_page < 1) demo_page = total_demos;
    }
    else if (keycode == SDLK_RIGHT) {
      demo_page++;
      if (demo_page > total_demos) demo_page = 1;
    }
  }
}

void init_render() {
  /* setup resources */
  tex_640x480 = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  tex_640x480.clear(Vec4(0, 0, 0, 1));
  tex_320x240 = sgl::create_texture(w / 2, h / 2, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  tex_320x240.clear(Vec4(0, 0, 0, 1));

  text_intro = sgl::read_file_as_wstring("assets/common/texts/introduction_to_Deep_Learning.txt");
  text_os437 = sgl::read_file_as_wstring("assets/common/texts/old_school_437_charset_demo.txt");
  text_chinese = sgl::read_file_as_wstring("assets/common/texts/chinese_long_text_sample.txt");
  sgl::replace_all(text_intro, L"\n", L" ");
  sgl::replace_all(text_chinese, L"\n", L"");
  sgl::replace_all(text_chinese, L"£¬", L"");
  sgl::replace_all(text_chinese, L"¡£", L"");
  sgl::replace_all(text_chinese, L"£»", L"");
  sgl::replace_all(text_chinese, L"¡¢", L"");
  text_chinese = sgl::repeat_string<std::wstring>(text_chinese, 2);

  Arial_11pt.load("assets/common/fonts/Arial/11pt_Regular.fnt");
  MiniHerz_16pt.load("assets/common/fonts/MiniHerz/16pt_Regular.fnt");
  Beatixel_16pt.load("assets/common/fonts/Beatixel/16pt_Regular.fnt");
  CuteBlockhead_16pt.load("assets/common/fonts/CuteBlockhead/16pt_Regular.fnt");
  GrapeSoda_16pt.load("assets/common/fonts/GrapeSoda/16pt_Regular.fnt");
  HP_100LX_6x8_8pt.load("assets/common/fonts/HP_100LX_6x8/8pt_Regular.fnt");
  IBM_CGA_8x8_8pt.load("assets/common/fonts/IBM_CGA_8x8/8pt_Regular.fnt");
  IBM_CGAthin_8x8_8pt.load("assets/common/fonts/IBM_CGAthin_8x8/8pt_Regular.fnt");
  IBM_EGA_8x14_16pt.load("assets/common/fonts/IBM_EGA_8x14/16pt_Regular.fnt");
  Superscript_16pt.load("assets/common/fonts/Superscript/16pt_Regular.fnt");
  Boutique_7pt.load("assets/common/fonts/Boutique_7pt/8pt_Regular.fnt");
  Boutique_9pt.load("assets/common/fonts/Boutique_9pt/11pt_Regular.fnt");
  Vonwaon_12pt.load("assets/common/fonts/Vonwaon_12pt/13pt_Regular.fnt");

  printf("Press left/right arrow ('<-'/'->') to switch between different demos.\n");
}

void demoinfo(const char* msg) {
  std::wstring message = L"Demo [" + std::to_wstring(demo_page) + L"/" + std::to_wstring(total_demos) + L"]: " + utf8string_to_wstring(msg);
  auto to_solid_block = [](const std::wstring& wstring) -> std::wstring {
    std::wstring new_string;
    new_string.resize(wstring.size(), L'\x2588'); /* solid block */
    return new_string;
  };
  Vec4 bg_color = Vec4(176, 176, 176, 255) / 255.0;
  Vec4 fg_color = Vec4(0, 0, 255, 255) / 255.0;
  HP_100LX_6x8_8pt.draw_text(&tex_640x480, to_solid_block(message), 0, h - 8, bg_color);
  HP_100LX_6x8_8pt.draw_text(&tex_640x480, message, 0, h - 8, fg_color);
}

double render_frame() {
  
  tex_640x480.clear(Vec4(0, 0, 0, 1));

  sgl::Timer timer;
  timer.tick();

  if (demo_page == 1) {
    HP_100LX_6x8_8pt.draw_text(&tex_640x480, text_os437, 0, 0, w, h / 2, Vec4(1, 1, 1, 1));
    IBM_CGAthin_8x8_8pt.draw_text(&tex_640x480, text_os437, 0, h / 2, w, h / 2, Vec4(1, 1, 1, 1));
    demoinfo("A simple text user interface demo featuring two monospaced fonts.");
  }
  else if (demo_page == 2) {
    HP_100LX_6x8_8pt.draw_text(&tex_640x480, text_intro, 0, 0, w, h, Vec4(1, 1, 1, 1));
    demoinfo("Using a small monospaced font to fill the entire screen for testing text rendering performance.");
  }
  else if (demo_page == 3) {
    Arial_11pt.draw_text(&tex_640x480, text_intro, 0, 0, w, h, Vec4(1, 1, 1, 1));
    demoinfo("Test for rendering unevenly spaced fonts (Character kerning, negative offsets, etc.).");
  }
  else if (demo_page == 4) {
    MiniHerz_16pt.draw_text(&tex_640x480, text_intro, 0, 0, w / 2, h / 2, Vec4(1, 1, 1, 1));
    Beatixel_16pt.draw_text(&tex_640x480, text_intro, w / 2, 0, w / 2, h / 2, Vec4(1, 1, 1, 1));
    CuteBlockhead_16pt.draw_text(&tex_640x480, text_intro, 0, h / 2, w / 2, h / 2, Vec4(1, 1, 1, 1));
    GrapeSoda_16pt.draw_text(&tex_640x480, text_intro, w / 2, h / 2, w / 2, h / 2, Vec4(1, 1, 1, 1));
    demoinfo("Test for rendering artistic-style fonts (MiniHerz, Beatixel, CuteBlockhead, GrapeSoda).");
  }
  else if (demo_page == 5) {
    Boutique_7pt.draw_text(&tex_640x480, text_chinese, 0, 0, w, h, Vec4(1, 1, 1, 1));
    demoinfo("Chinese character display demo (7pt).");
  }
  else if (demo_page == 6) {
    Boutique_9pt.draw_text(&tex_640x480, text_chinese, 0, 0, w, h, Vec4(1, 1, 1, 1));
    demoinfo("Chinese character display demo (9pt).");
  }
  else if (demo_page == 7) {
    Vonwaon_12pt.draw_text(&tex_640x480, text_chinese, 0, 0, w, h, Vec4(1, 1, 1, 1));
    demoinfo("Chinese character display demo (12pt).");
  }
  else if (demo_page == 8) {
    Superscript_16pt.draw_text(&tex_320x240, text_intro, 1, 1, w / 2, h / 2, Vec4(64, 64, 64) / 255.0);
    Superscript_16pt.draw_text(&tex_320x240, text_intro, 0, 0, w / 2, h / 2, Vec4(192, 192, 192) / 255.0);
    sgl::blit_texture(&tex_320x240, &tex_640x480, 0, 0, w / 2, h / 2, 0, 0, w, h);
    demoinfo("Another text display demo (2x zoomed display).");
  }

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
    T_frame += render_frame();
    frameid++;

    sgl::SDL2::sgl_texture_to_SDL2_surface(&tex_640x480, pWindowSurface);
    SDL_UpdateWindowSurface(pWindow);
    std::string title = std::string("SGL | ") + dtos(T_frame / frameid * 1000.0, 2) + "ms | FPS=" + std::to_string(int(frameid / T_frame));
    SDL_SetWindowTitle(pWindow, title.c_str());
  }

  destroy_env();
  return 0;
}
