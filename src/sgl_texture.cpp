#include <malloc.h>
#include "sgl_texture.h"
#include "sgl_utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace sgl {

Texture::Texture() {
  w = h = 0;
  pixels = NULL;
  format = PixelFormat::pixel_format_unknown;
  sampling = TextureSampling::texture_sampling_point;
}

void
Texture::destroy() {
  this->w = 0;
  this->h = 0;
  if (this->pixels)
    free(this->pixels);
  this->pixels = NULL;
  this->format = PixelFormat::pixel_format_unknown;
}

void
Texture::create(int32_t w, int32_t h, 
  PixelFormat texture_format,
  TextureSampling texture_sampling) {
  this->destroy();
  if (w <= 0 || h <= 0)
    return;
  this->w = w;
  this->h = h;
  this->format = texture_format;
  this->sampling = texture_sampling;
  this->bypp = 0; /* set a default value here */
  if (texture_format == PixelFormat::pixel_format_RGBA8888 ||
    texture_format == PixelFormat::pixel_format_BGRA8888) {
    this->bypp = 4;
  }
  else if (texture_format == PixelFormat::pixel_format_float64) {
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
Texture::texture_RGBA8888_point(const Vec2 &p) const {
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

Vec4 Texture::texture_BGRA8888_point(const Vec2 & p) const
{
  /* point (nearest) sampling */
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);
  int x = min(int(p0.x * w), w - 1);
  int y = min(int(p0.y * h), h - 1);

  int pixel_id = y * w + x;
  uint8_t *data = (uint8_t *)pixels;
  uint8_t R = data[pixel_id * 4 + 2];
  uint8_t G = data[pixel_id * 4 + 1];
  uint8_t B = data[pixel_id * 4 + 0];
  uint8_t A = data[pixel_id * 4 + 3];

  return Vec4(R, G, B, A) / 255.0;
}

Texture Texture::to_format(const PixelFormat & target_format) const
{
  if (this->format == target_format) {
    return (*this);
  }
  Texture converted_texture;
  converted_texture.create(this->w, this->h, target_format, this->sampling);
  uint8_t* dst = (uint8_t*)converted_texture.pixels;
  uint8_t* src = (uint8_t*)this->pixels;
  if (this->format == PixelFormat::pixel_format_RGBA8888 &&
    target_format == PixelFormat::pixel_format_BGRA8888) {
    for (int y = 0; y < this->h; y++) {
      for (int x = 0; x < this->w; x++) {
        int pid = y * this->w + x;
        dst[pid * 4 + 2] = src[pid * 4 + 0];
        dst[pid * 4 + 1] = src[pid * 4 + 1];
        dst[pid * 4 + 0] = src[pid * 4 + 2];
        dst[pid * 4 + 3] = src[pid * 4 + 3];
      }
    }
  }
  else if (this->format == PixelFormat::pixel_format_BGRA8888 &&
    target_format == PixelFormat::pixel_format_RGBA8888) {
    for (int y = 0; y < this->h; y++) {
      for (int x = 0; x < this->w; x++) {
        int pid = y * this->w + x;
        dst[pid * 4 + 2] = src[pid * 4 + 0];
        dst[pid * 4 + 1] = src[pid * 4 + 1];
        dst[pid * 4 + 0] = src[pid * 4 + 2];
        dst[pid * 4 + 3] = src[pid * 4 + 3];
      }
    }
  }
  else {
    printf("Unimplemented texture format conversion type.\n");
  }


  return converted_texture;
}

bool Texture::save_png(const std::string & path) const
{
  if (this->w <= 0 || this->h <= 0 || this->pixels == NULL) {
    printf("Cannot save texture, texture object is invalid.\n");
    return false;
  }
  if (this->bypp != 4 || this->format == PixelFormat::pixel_format_float64 || 
    this->format == PixelFormat::pixel_format_unknown) {
    printf("Cannot save texture, unsupported pixel format.\n");
    return false;
  }
  /* stb image default to RGBA format */
  if (this->format != PixelFormat::pixel_format_RGBA8888) {
    Texture texobj = this->to_format(PixelFormat::pixel_format_RGBA8888);
    return texobj.save_png(path);
  }
  else {
    if (stbi_write_png(path.c_str(), this->w, this->h, 4, this->pixels, this->w * 4) == 0) {
      printf("Cannot save texture, stbi_write_png failed.\n");
      return false;
    }
    else 
      return true;
  }
  return false;
}

Texture
load_texture(const std::string &file, const PixelFormat& target_format) {
  int x, y, n;
  unsigned char *data = stbi_load(file.c_str(), &x, &y, &n, 4);
  Texture texture;
  if (data == NULL) {
    const char *failure = stbi_failure_reason();
    printf("Failed to load image \"%s\", %s.\n", file.c_str(), failure);
    printf("* note: current working directory is: \"%s\".\n", get_cwd().c_str());
    return texture;
  }
  texture.create(x, y, PixelFormat::pixel_format_RGBA8888, TextureSampling::texture_sampling_point);
  uint8_t *pixels = (uint8_t *) texture.pixels;
  memcpy(pixels, data, x * y * 4);
  stbi_image_free(data);
  return texture.to_format(target_format);
}

Vec4
texture(const Texture *texobj, const Vec2 &uv) {
  if (texobj->format == PixelFormat::pixel_format_RGBA8888) {
    if (texobj->sampling == TextureSampling::texture_sampling_point) {
      return texobj->texture_RGBA8888_point(uv);
    }
  }
  else if (texobj->format == PixelFormat::pixel_format_BGRA8888) {
    if (texobj->sampling == TextureSampling::texture_sampling_point) {
      return texobj->texture_BGRA8888_point(uv);
    }
  }
  return Vec4(0, 0, 0, 0);
}

}; /* namespace sgl */
