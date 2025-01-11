#pragma once

/**
Defines and implements:
  * common 2D drawing operations.
  * 3D primitive meshes generation.
  * bitmap font rendering.
2D shapes are rendered directly onto the target texture, while 
3D primitives return a generated mesh for further use.
**/

#include "sgl_utils.h"
#include "sgl_math.h"
#include "sgl_texture.h"

namespace sgl {
 
/*
The internal bitmap font used by this library is generated from a
third party tool "Bitmap Font Generator" by AngelCode. The tool
can be downloaded from https://www.angelcode.com/products/bmfont/,
which can generate tiled bitmap font glyphs from TrueType fonts.

Fonts used by this library are downloaded from
  * https://www.pentacom.jp/pentacom/bitfontmaker2/gallery/,
    which offers a variety of high-quality pixelated fonts.
  * https://int10h.org/oldschool-pc-fonts/download/,
    providing a selection of classic old-school PC fonts.
  * https://www.dafont.com/bitmap.php,
    which features a collection of high-quality bitmap fonts.
*/
class Font {
public:
  struct Glyph {
    /*
    See
    https://www.angelcode.com/products/bmfont/doc/render_text.html
    for the definition of the above data members and how to display
    a character glyph onto texture properly.

    NOTE: If a glyph does not contain any valid pixels (all black), 
          the 'is_empty' member will be set to 1; otherwise, it will 
          be set to 0.
    */
    uint32_t unicode;
    int8_t xoffset, yoffset, xadvance, is_empty;
    sgl::Texture tex;
  };

protected:
  std::map<uint32_t, Glyph> charmap;
  std::map<std::pair<uint32_t, uint32_t>, int32_t> kernings; /* (cur_charcode, prev_charcode) -> kerning amount */
  int32_t font_size, line_height, line_base;
  uint8_t is_bold, is_italic;
  std::string face_name;

public:
  bool load(const char* path);
  void unload();

  /*
  Render text onto texture.
  When target == NULL, the function calculates the text extent instead of rendering
  text onto a target. It returns the final cursor position (x, y). Users can ignore
  the return value if they only need to render text onto textures.
  */
  IVec2 draw_text(sgl::Texture* target, const std::wstring & text, int x, int y, const Vec4& color);
  IVec2 draw_text(sgl::Texture* target, const std::wstring & text, int x, int y, int w, int h, const Vec4& color);

  /*
  Computes the width and height of the specified string of text.
  w and h represents the width and height of the text box.
  */
  IVec2 get_text_extent_point(const std::wstring & text);
  IVec2 get_text_extent_point(const std::wstring & text, int w, int h);

  void set_line_height(int new_height);

public:
  Font();
  virtual ~Font();

protected:
  bool _load_from_BitmapFontGenerator(const char* path);

};

/* draw primitives onto texture directly */
void draw_pixel(sgl::Texture* target, int x, int y, const Vec4& color);
void draw_line(sgl::Texture* target, int x1, int y1, int x2, int y2, const Vec4& color);
void draw_circle(sgl::Texture* target, int x, int y, int r, const Vec4& color);
void draw_ellipse(sgl::Texture* target, double x, double y, double rx, double ry, double rotation, const Vec4& color, int nsegs);
void draw_rectangle(sgl::Texture* target, int x, int y, int w, int h, const Vec4& color);
void draw_rectangle(sgl::Texture* target, double cx, double cy, double w, double h, double rotation, const Vec4& color);
void draw_bezier(sgl::Texture* target, const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, const Vec4& color, int nsegs);
void draw_bezier2(sgl::Texture* target, const Vec2& p1, const Vec2& p1_tangent, const Vec2& p2, const Vec2& p2_tangent, const Vec4& color, int nsegs);

/* font rendering */
void draw_text(sgl::Texture* target, sgl::Font* font, const std::wstring& text, int x, int y, const Vec4& color);
void draw_text(sgl::Texture* target, sgl::Font* font, const std::wstring& text, int x, int y, int w, int h, const Vec4& color);
IVec2 get_text_extent_point(sgl::Font* font, const std::wstring & text);
IVec2 get_text_extent_point(sgl::Font* font, const std::wstring & text, int w, int h);

};
