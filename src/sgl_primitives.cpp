#include "sgl_primitives.h"

namespace sgl {

void draw_pixel(sgl::Texture * target, int x, int y, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 || 
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (x < 0 || x >= target->get_width() || y < 0 || y >= target->get_height())
    return;
  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);
  pixels[y * target->get_width() + x] = packed_color;
}

void draw_line(sgl::Texture * target, int x1, int y1, int x2, int y2, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  
  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  int dx, dy;
  int x, y;
  int epsilon = 0;
  int w = target->get_width(), h = target->get_height();
  int Dx = x2 - x1;
  int Dy = y1 - y2;
  Dx > 0 ? dx = +1 : dx = -1;
  Dy > 0 ? dy = -1 : dy = +1;
  Dx = ::abs(Dx), Dy = ::abs(Dy);
  if (Dx > Dy) {
    y = y1;
    for (x = x1; x != x2 && x < w; x += dx) {
      /* draw (x, y) here */
      if (x >= 0 && y >= 0 && y < h)
        pixels[y * w + x] = packed_color;
      /* prepare for next iteration */
      epsilon += Dy;
      if ((epsilon << 1) > Dx) {
        y += dy;
        epsilon -= Dx;
      }
    }
  }
  else {
    x = x1;
    for (y = y1; y != y2 && y < h; y += dy) {
      /* draw (x, y) here */
      if (x >= 0 && x < w && y >= 0)
        pixels[y * w + x] = packed_color;
      /* prepare for next iteration */
      epsilon += Dx;
      if ((epsilon << 1) > Dy) {
        epsilon -= Dy;
        x += dx;
      }
    }
  }
}

void draw_circle(sgl::Texture * target, int x, int y, int r, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);
  
  /* define a simple auxiliary function for writing pixels to texture */
  int w = target->get_width(), h = target->get_height();
  auto write = [&](int x0, int y0){
    if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h)
      pixels[y0 * w + x0] = packed_color;
  };

  /*
  an implementation of the midpoint circle drawing algorithm, see
    * https://www.youtube.com/watch?v=hpiILbMkF9w, and
    * https://www.geeksforgeeks.org/mid-point-circle-drawing-algorithm/
  for more info.
  */

  int dx = r, dy = 0;
  /* the initial point on the axes after translation */
  write(x + dx, y + dy);
  /* When radius is zero only a single point will be printed */
  if (r > 0) {
    write(x - dx, y + dy);
    write(x + dy, y - dx);
    write(x + dy, y + dx);
  }

  /* Initialising the value of P */
  int P = 1 - r;
  while (dx > dy)
  {
    dy++;
    /* Mid-point is inside or on the perimeter */
    if (P <= 0) 
      P = P + 2 * dy + 1;
    /* Mid-point is outside the perimeter */
    else {
      dx--;
      P = P + 2 * dy - 2 * dx + 1;
    }
    /* All the perimeter points have already been printed */
    if (dx < dy)
      break;
    /* 
    Printing the generated point and its reflection
    in the other octants after translation
    */
    write(x + dx, y + dy);
    write(x - dx, y + dy);
    write(x + dx, y - dy);
    write(x - dx, y - dy);
    /*
    If the generated point is on the line x = y then 
    the perimeter points have already been printed
    */
    if (dx != dy) {
      write(x + dy, y + dx);
      write(x - dy, y + dx);
      write(x + dy, y - dx);
      write(x - dy, y - dx);
    }
  }
}

void draw_ellipse(sgl::Texture* target, double x, double y, double rx, double ry, double rotation, const Vec4& color, int nsegs)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (nsegs < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  /* auxiliary function for drawing line from (x1, y1) to (x2, y2) */
  int w = target->get_width(), h = target->get_height();
  auto bresenham = [&](int x1, int y1, int x2, int y2) {
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < w; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < h)
          pixels[y * w + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < h; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < w && y >= 0)
          pixels[y * w + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  double yx_ratio = (double)ry / (double)rx;
  double angle = 0.0;
  double step = 2 * sgl::PI / (double)nsegs;

  Vec2 start = Vec2(x, y) + sgl::rotate(Vec2(0, -ry), rotation);
  Vec2 end;
  Vec2 scale = Vec2(1.0, yx_ratio);
  for (int i = 0; i < nsegs; i++) {
    Vec2 dxy = sgl::rotate(scale * sgl::rotate(Vec2(0, -rx), (i + 1) * step), rotation);
    if (i < nsegs - 1)
      end = Vec2(x, y) + dxy;
    else
      end = Vec2(x, y) + sgl::rotate(Vec2(0, -ry), rotation);
    bresenham((int)start.x, (int)start.y, (int)end.x, (int)end.y);
    start = end;
  }
}

void draw_rectangle(sgl::Texture * target, int x, int y, int w, int h, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (w < 1 || h < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  /* define a simple auxiliary function for writing pixels to texture */
  int tw = target->get_width(), th = target->get_height();
  auto write = [&](int x0, int y0) {
    if (x0 >= 0 && x0 < tw && y0 >= 0 && y0 < th)
      pixels[y0 * tw + x0] = packed_color;
  };

  for (int curx = x; curx < x + w; curx++) {
    write(curx, y);
    write(curx, y + h - 1);
  }
  for (int cury = y; cury < y + h; cury++) {
    write(x, cury);
    write(x + w - 1, cury);
  }
}

void draw_rectangle(sgl::Texture * target, double cx, double cy, double w, double h, double rotation, const Vec4 & color)
{
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */
  if (w < 1 || h < 1) return;

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);


  /* define four corners */
  double dx = w / 2.0, dy = h / 2.0;
  Vec2 p[4];
  Vec2 o = Vec2(cx, cy);
  p[0] = o + sgl::rotate(Vec2(-dx, -dy), rotation);
  p[1] = o + sgl::rotate(Vec2(+dx, -dy), rotation);
  p[2] = o + sgl::rotate(Vec2(+dx, +dy), rotation);
  p[3] = o + sgl::rotate(Vec2(-dx, +dy), rotation);

  /* auxiliary function for drawing line from (x1, y1) to (x2, y2) */
  int tw = target->get_width(), th = target->get_height();
  auto bresenham = [&](int x1, int y1, int x2, int y2) {
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < tw; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < th)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < th; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < tw && y >= 0)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  bresenham((int)p[0].x, (int)p[0].y, (int)p[1].x, (int)p[1].y);
  bresenham((int)p[1].x, (int)p[1].y, (int)p[2].x, (int)p[2].y);
  bresenham((int)p[2].x, (int)p[2].y, (int)p[3].x, (int)p[3].y);
  bresenham((int)p[3].x, (int)p[3].y, (int)p[0].x, (int)p[0].y);
}

void draw_bezier(sgl::Texture * target, const Vec2 & p0, const Vec2 & p1, const Vec2 & p2, const Vec2 & p3, const Vec4 & color, int nsegs)
{
  /*
  See
  https://stackoverflow.com/questions/785097/how-do-i-implement-a-b%C3%A9zier-curve-in-c
  for more infomation about drawing B¨¦zier curves in C.
  */
  if (target == NULL || target->get_width() <= 0 || target->get_height() <= 0 ||
    target->get_bytes_per_pixel() != 4) return; /* only supports 32 bit texture */

  uint32_t* pixels = (uint32_t*)target->get_pixel_data();
  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  double t_step = 1.0 / nsegs;
  Vec2 Q_last, Q;
  
  auto interpolate = [](Vec2 a, Vec2 b, double t) { return a * (1.0 - t) + b * t; };
  auto line_to_Q = [&](){
    /* Draw a line from Q_last to Q, both positions of Q_last and Q are maintained 
    properly from the main for loop. */
    int x1 = (int)Q_last.x, x2 = (int)Q.x;
    int y1 = (int)Q_last.y, y2 = (int)Q.y;
    int tw = target->get_width(), th = target->get_height();
    int dx, dy;
    int x, y;
    int epsilon = 0;
    int Dx = x2 - x1;
    int Dy = y1 - y2;
    Dx > 0 ? dx = +1 : dx = -1;
    Dy > 0 ? dy = -1 : dy = +1;
    Dx = ::abs(Dx), Dy = ::abs(Dy);
    if (Dx > Dy) {
      y = y1;
      for (x = x1; x != x2 && x < tw; x += dx) {
        /* draw (x, y) here */
        if (x >= 0 && y >= 0 && y < th)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dy;
        if ((epsilon << 1) > Dx) {
          y += dy;
          epsilon -= Dx;
        }
      }
    }
    else {
      x = x1;
      for (y = y1; y != y2 && y < th; y += dy) {
        /* draw (x, y) here */
        if (x >= 0 && x < tw && y >= 0)
          pixels[y * tw + x] = packed_color;
        /* prepare for next iteration */
        epsilon += Dx;
        if ((epsilon << 1) > Dy) {
          epsilon -= Dy;
          x += dx;
        }
      }
    }
  };

  for (int i = 0; i <= nsegs; i++) {
    double t = i * t_step;
    Vec2 P01 = interpolate(p0, p1, t);
    Vec2 P12 = interpolate(p1, p2, t);
    Vec2 P23 = interpolate(p2, p3, t);
    Vec2 P0112 = interpolate(P01, P12, t);
    Vec2 P1223 = interpolate(P12, P23, t);
    Q = interpolate(P0112, P1223, t);
    if (i > 0)
      line_to_Q();
    Q_last = Q;
  }
}

void draw_bezier2(sgl::Texture * target, const Vec2 & p1, const Vec2 & p1_tangent, const Vec2 & p2, const Vec2 & p2_tangent, const Vec4 & color, int nsegs)
{
  Vec2 P0 = p1;
  Vec2 P1 = p1 + p1_tangent;
  Vec2 P2 = p2;
  Vec2 P3 = p2 + p2_tangent;
  draw_bezier(target, P0, P1, P2, P3, color, nsegs);
}

void draw_text(sgl::Texture * target, sgl::Font * font, const std::wstring & text, int x, int y, const Vec4& color)
{
  font->draw_text(target, text, x, y, color);
}

void draw_text(sgl::Texture * target, sgl::Font * font, const std::wstring & text, int x, int y, int w, int h, const Vec4 & color)
{
  font->draw_text(target, text, x, y, w, h, color);
}

IVec2 get_text_extent_point(sgl::Font * font, const std::wstring & text)
{
  return font->get_text_extent_point(text);
}

IVec2 get_text_extent_point(sgl::Font * font, const std::wstring & text, int w, int h)
{
  return font->get_text_extent_point(text, w, h);
}

bool Font::load(const char * path)
{
  unload();

  if (sgl::endswith(path, ".fnt")) {
    /* 
    the font is generated from Bitmap Font Generator.
    */
    if (!_load_from_BitmapFontGenerator(path))
      unload();
    else return true;
  }
  else {
    printf("Cannot load font from path \"%s\". Unknown font format.\n", path);
  }
  return false;
}

void Font::unload()
{
  charmap.clear();
  kernings.clear();
  line_height = 0;
  line_base = 0;
  is_bold = 0;
  is_italic = 0;
  font_size = 0;
  face_name = "";
}

IVec2 Font::draw_text(sgl::Texture * target, const std::wstring & text, int x, int y, int w, int h, const Vec4 & color)
{
  if (target != NULL) {
    if (target->get_width() <= 0 || target->get_height() <= 0 || target->get_bytes_per_pixel() != 4) {
      printf("Invalid texture bit depth or size configuration.\n");
      return IVec2(0, 0);
    }
    if (target->get_pixel_format() != PixelFormat_BGRA8888 && target->get_pixel_format() != PixelFormat_RGBA8888) {
      printf("Invalid texture format. Bitmap glyph can only be drawn onto texture with RGBA8888 or BGRA8888 format.\n");
      return IVec2(0, 0);
    }
  }
  if (text.size() == 0)
    return IVec2(x, y);

  uint8_t R, G, B, A;
  uint32_t packed_color;
  convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
  if (target != NULL)
    pack_RGBA8888_to_uint32(R, G, B, A, target->get_pixel_format(), packed_color);

  /* define an auxiliary function for blitting a single glyph onto target texture */
  auto blit_glyph_to_target = [](const Glyph& glyph, sgl::Texture* target,
    int x_dst, int y_dst, const uint32_t& packed_color) -> void
  {
    if (target == NULL) return;
    int w_src = glyph.tex.get_width(), h_src = glyph.tex.get_height();
    int w_dst = target->get_width(), h_dst = target->get_height();
    uint8_t* glyph_data = (uint8_t*)glyph.tex.get_pixel_data();
    uint32_t* target_data = (uint32_t*)target->get_pixel_data();
    for (int y = y_dst; y < y_dst + h_src; y++) {
      for (int x = x_dst; x < x_dst + w_src; x++) {
        int x0 = x - x_dst, y0 = y - y_dst;
        bool dst_valid = (x >= 0 && x < target->get_width() && y >= 0 && y < target->get_height());
        bool src_allow_get = (glyph_data[y0 * w_src + x0] > 0);
        if (!dst_valid || !src_allow_get) continue;
        target_data[y * w_dst + x] = packed_color;
      }
    }
  };

  /* render a single line text */
  int x_cursor = 0, y_cursor = 0;
  bool cursor_inited = false;
  uint32_t line_chars = 0; /* number of blitted chars in current line */
  /*
  (x_dst, y_dst) represents the upper-left corner position of the glyph 
  when it is about to be blitted onto the target texture.
  */
  int x_dst, y_dst; 

  /* 
  Auxiliary function for manipulating cursor position.
  */
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
      if (cursor_exceeds_height_limit)
        /* Exit early as the text exceeds the boundaries of the text box. */
        return IVec2(x_cursor, y_cursor);
    }
    /*
    Render glyph to texture.
    */
    blit_glyph_to_target(glyph, target, x_dst, y_dst, packed_color);
    line_chars++;
    x_cursor += glyph.xadvance;
  }

  return IVec2(x_cursor, y_cursor);
}

IVec2 Font::draw_text(sgl::Texture * target, const std::wstring & text, int x, int y, const Vec4& color)
{
  return draw_text(target, text, x, y, 0, 0, color);
}

Font::Font()
{
  unload();
}

Font::~Font()
{
  unload();
}

bool Font::_load_from_BitmapFontGenerator(const char * path)
{
  /* 
  The Bitmap Font Generator will produce two files:
    1. A "*.fnt" file containing the glyph information,
    2. A bitmap file representing the actual font glyph tiles. 
  */
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

  /* Some bitmap fonts can be stored in multiple pages. */
  std::map<int32_t, sgl::Texture> page_id2tex; /* page id to bitmap texture */

  const int bufsize = 1024;
  char buf[bufsize];
  memset(buf, 0, bufsize);
  int lineno = 0;
  while (fgets(buf, bufsize - 1, fp) != NULL) {
    lineno++;
    std::string line = regularize_string(buf);
    std::vector<std::string> tokens = sgl::split(line, " ");
    std::string name, value;
    if (tokens[0] == "info") {
      for (int itok = 1; itok < tokens.size(); itok++) {
        read_config(tokens[itok], name, value);
        if (name == "face") {
          sgl::replace_all(value, "\"", ""); /* remove '"' */
          this->face_name = value;
        }
        else if (name == "size") {
          int font_size = atoi(value.c_str());
          this->font_size = font_size < 0 ? -font_size : font_size;
        }
        else if (name == "bold") {
          this->is_bold = atoi(value.c_str());
        }
        else if (name == "italic") {
          this->is_italic = atoi(value.c_str());
        }
      }
    }
    else if (tokens[0] == "common") {
      for (int itok = 1; itok < tokens.size(); itok++) {
        read_config(tokens[itok], name, value);
        if (name == "lineHeight") {
          this->line_height = atoi(value.c_str());
        }
        else if (name == "base") {
          this->line_base = atoi(value.c_str());
        }
      }
    }
    else if (tokens[0] == "page") {
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
      sgl::Texture tex;
      if (!file_exists(texfile)) {
        printf("Cannot open file \"%s\", file not exist.\n", texfile.c_str());
      }
      else {
        tex = sgl::load_texture(texfile, PixelFormat_UInt8, TextureSampling_Nearest);
      }
      page_id2tex.insert_or_assign(page_id, tex);
    }
    else if (tokens[0] == "char") {
      Glyph new_glyph;
      int32_t glyph_page_id, x, y, w, h;
      for (int itok = 1; itok < tokens.size(); itok++) {
        read_config(tokens[itok], name, value);
        if (name == "id") new_glyph.unicode = atoi(value.c_str());
        else if (name == "xoffset") new_glyph.xoffset = atoi(value.c_str());
        else if (name == "yoffset") new_glyph.yoffset = atoi(value.c_str());
        else if (name == "xadvance") new_glyph.xadvance = atoi(value.c_str());
        else if (name == "page") glyph_page_id = atoi(value.c_str());
        else if (name == "x") x = atoi(value.c_str());
        else if (name == "y") y = atoi(value.c_str());
        else if (name == "width") w = atoi(value.c_str());
        else if (name == "height") h = atoi(value.c_str());
      }
      if (page_id2tex.find(glyph_page_id) == page_id2tex.end()) {
        printf("[Line #%d] Invalid glyph page id \"%d\". The required page is still not defined before this glyph.\n", lineno, glyph_page_id);
      }
      if (w > 0 && h > 0) {
        new_glyph.tex = sgl::create_texture(w, h, PixelFormat_UInt8, TextureSampling_Nearest, TextureUsage_ColorComponents);
      }
      else {
        printf("[Line #%d] Invalid glyph size config (w<=0 or h<=0).\n", lineno);
      }
      /* now blit texture data to glyph */
      sgl::blit_texture(&page_id2tex[glyph_page_id], &new_glyph.tex, x, y, w, h, 0, 0);
      /* check if this glyph is all black (empty glyph), we will use this information
      when drawing the text. */
      uint8_t* glyph_data_ptr = (uint8_t*)new_glyph.tex.get_pixel_data();
      new_glyph.is_empty = 1;
      for (size_t ipx = 0; ipx < w * h; ipx++) {
        if (glyph_data_ptr[ipx] > 0) {
          new_glyph.is_empty = 0;
          break;
        }
      }
      /* finally, add glyph to charmap */
      this->charmap.insert_or_assign(new_glyph.unicode, new_glyph);
    }
    else if (tokens[0] == "kerning") {
      uint32_t first, second;
      int32_t amount;
      for (int itok = 1; itok < tokens.size(); itok++) {
        read_config(tokens[itok], name, value);
        if (name == "first") first = (uint32_t)atoi(value.c_str());
        else if (name == "second") second = (uint32_t)atoi(value.c_str());
        else if (name == "amount") amount = (int32_t)atoi(value.c_str());
      }
      std::pair<uint32_t,uint32_t> kerning_pair = std::make_pair(second, first);
      this->kernings.insert_or_assign(kerning_pair, amount);
    }
  }
  fclose(fp);
  return true;
}

void Font::set_line_height(int new_height)
{
  this->line_height = new_height;
}

IVec2 Font::get_text_extent_point(const std::wstring & text)
{
  return this->draw_text(NULL, text, 0, 0, Vec4(1, 1, 1, 1));
}

IVec2 Font::get_text_extent_point(const std::wstring & text, int w, int h)
{
  return this->draw_text(NULL, text, 0, 0, w, h, Vec4(1, 1, 1, 1));
}

};


