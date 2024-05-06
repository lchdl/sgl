#include <malloc.h>
#include "sgl_texture.h"
#include "sgl_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace sgl {

Texture::Texture() {
  w = h = 0;
  pixels = NULL;
  format = TextureFormat::texture_format_invalid;
  sampling = TextureSampling::texture_sampling_point;
}

void
Texture::destroy() {
  this->w = 0;
  this->h = 0;
  if (this->pixels)
    free(this->pixels);
  this->pixels = NULL;
  this->format = TextureFormat::texture_format_invalid;
}

void
Texture::create(int32_t w, int32_t h, TextureFormat texture_format,
                TextureSampling texture_sampling) {
  this->destroy();
  if (w <= 0 || h <= 0)
    return;
  this->w = w;
  this->h = h;
  this->format = texture_format;
  this->sampling = texture_sampling;
  this->bypp = 0; /* set a default value here */
  if (texture_format == TextureFormat::texture_format_RGBA8) {
    this->bypp = 4;
  }
  else if (texture_format == TextureFormat::texture_format_float64) {
    this->bypp = 8;
  }
  else {
    printf("Texture create failed: unsupported / "
      "unimplemented texture format.\n");
  }
  this->pixels = malloc(w * h * bypp);
}

void
Texture::copy(const Texture &texture) {
  this->create(texture.w, texture.h, texture.format, texture.sampling);
  if (this->pixels != NULL && texture.pixels != NULL) {
    memcpy(this->pixels, texture.pixels, w * h * bypp);
  }
}

Texture::~Texture() { destroy(); }

Texture::Texture(const Texture &texture) {
  this->pixels = NULL;
  copy(texture);
}

Texture &
Texture::operator=(const Texture &texture) {
  if (this == &texture)
    return (*this);
  copy(texture);
  return (*this);
}

Vec4
Texture::texture_RGBA8_point(const Vec2 &p) const {
  /* point (nearest) sampling */
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);
  int x = min(int(p0.x * w), w - 1);
  int y = min(int(p0.y * h), h - 1);

  int pixel_id = y * w + x;
  uint8_t *data = (uint8_t *) pixels;
  uint8_t R = data[pixel_id * 4 + 0];
  uint8_t G = data[pixel_id * 4 + 1];
  uint8_t B = data[pixel_id * 4 + 2];
  uint8_t A = data[pixel_id * 4 + 3];

  return Vec4(R, G, B, A) / 255.0;
}

Texture
load_texture(const std::string &file) {
  int x, y, n;
  unsigned char *data = stbi_load(file.c_str(), &x, &y, &n, 4);
  Texture texture;
  if (data == NULL) {
    const char *failure = stbi_failure_reason();
    printf("Failed to load image \"%s\", %s.\n", file.c_str(), failure);
		printf("* note: current working directory is: \"%s\".\n", get_cwd().c_str());
		return texture;
  }
  texture.create(x, y, TextureFormat::texture_format_RGBA8,
                 TextureSampling::texture_sampling_point);
  uint8_t *pixels = (uint8_t *) texture.pixels;
  memcpy(pixels, data, x * y * 4);
  stbi_image_free(data);
  return texture;
}

Vec4
texture(const Texture *texobj, const Vec2 &uv) {
  if (texobj->format == TextureFormat::texture_format_RGBA8) {
    if (texobj->sampling == TextureSampling::texture_sampling_point) {
      return texobj->texture_RGBA8_point(uv);
    }
  }
  return Vec4(0, 0, 0, 0);
}

};   // namespace sgl
