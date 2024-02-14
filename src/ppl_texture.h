#pragma once
#include "ppl_math.h"
#include <stdint.h>
#include <string>

/* ppl texture uses STB image for image loading/saving. */

namespace ppl {
class Texture {
public:
  int32_t w, h, bypp;
  void *pixels;

public:
  /**
  Create an empty texture.
  @param w, h: width and height of the texture (in pixels).
  @param bypp: Bytes per pixel. For a RGBA8 texture, bypp=4, for a texture
  storing float64, simply set it bypp=8.
  **/
  void create(int32_t w, int32_t h, int32_t bypp);
  /**
  Destroy texture.
  **/
  void destroy();
  /**
  Texture sampling (nearest sampling).
  @param p: Normalized texture coordinate. Origin is located at the lower-left
  corner, with +x pointing to the left and +y pointing to the top. Out of bound
  values will be clamped to [0, 1] before texturing.
  **/
  Vec4 texture_RGBA8(const Vec2 &p) const;

protected:
  /**
  Internal copy from another texture.
  **/
  void copy(const Texture &texture);

public:
  Texture();
  ~Texture();
  Texture(const Texture &texture);
  Texture &operator=(const Texture &texture);
};

/**
  Load an image from disk and return the loaded texture object. The loaded
texture will always has the RGBA8 format.
  @param file: Image file path.
  @returns: The loaded image texture. If image loading failed, an empty texture
will be returned (pixels=NULL).
**/
Texture load_image_as_texture(const std::string &file);

};   // namespace ppl
