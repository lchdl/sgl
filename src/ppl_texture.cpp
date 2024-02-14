#include "ppl_texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <malloc.h>

namespace ppl {

Texture::Texture() {
  w = h  = 0;
  pixels = NULL;
}

Texture::~Texture() { destroy(); }

void
Texture::create(int32_t w, int32_t h, int32_t bypp) {
  this->w      = w;
  this->h      = h;
  this->bypp   = bypp;
  this->pixels = malloc(w * h * bypp);
}

void
Texture::destroy() {
  this->w = 0;
  this->h = 0;
  if (this->pixels)
    free(this->pixels);
  this->pixels = NULL;
}

Texture::Texture(const Texture &texture) { copy(texture); }

Texture &
Texture::operator=(const Texture &texture) {
  if (this == &texture)
    return (*this);
  copy(texture);
  return (*this);
}

void
Texture::copy(const Texture &texture) {
  this->destroy();
  if (texture.pixels == NULL || texture.w <= 0 || texture.h <= 0)
    return;
  this->create(texture.w, texture.h, texture.bypp);
  memcpy(this->pixels, texture.pixels, w * h * bypp);
}

Vec4
Texture::texture_RGBA8(const Vec2 &p) const {
  /* nearest sampling */

  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);

  int x = min(int(p0.x * w), w - 1);
  int y = min(int(p0.y * h), h - 1);

  int pixel_id  = y * w + x;
  uint8_t *data = (uint8_t *) pixels;
  uint8_t R     = data[pixel_id * 4 + 0];
  uint8_t G     = data[pixel_id * 4 + 1];
  uint8_t B     = data[pixel_id * 4 + 2];
  uint8_t A     = data[pixel_id * 4 + 3];
  return Vec4(R, G, B, A) / 255.0;
}

Texture
load_image_as_texture(const std::string &file) {
  int x, y, n;
  unsigned char *data = stbi_load(file.c_str(), &x, &y, &n, 4);
  Texture texture;
  if (data == NULL) {
    const char *failure = stbi_failure_reason();
    printf("Failed to load image \"%s\", %s.\n", file.c_str(), failure);
    return texture;
  }
  texture.create(x, y, 4);
  uint8_t *pixels = (uint8_t *) texture.pixels;
  memcpy(pixels, data, x * y * 4);
  stbi_image_free(data);
  return texture;
}

};   // namespace ppl
