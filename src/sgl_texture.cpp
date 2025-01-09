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
  format = PixelFormat_Unknown;
  sampling = TextureSampling_Nearest;
  usage = TextureUsage_Unknown;
}

void Texture::destroy() {
  this->w = 0;
  this->h = 0;
  if (this->pixels)
    free(this->pixels);
  this->pixels = NULL;
  this->format = PixelFormat_Unknown;
  this->sampling = TextureSampling_Nearest;
  this->usage = TextureUsage_Unknown;
}

void Texture::create(int32_t w, int32_t h, PixelFormat texture_format, TextureSampling texture_sampling, TextureUsage texture_usage) {
  this->destroy();
  if (w <= 0 || h <= 0)
    return;
  this->w = w;
  this->h = h;
  this->format = texture_format;
  this->sampling = texture_sampling;
  this->usage = texture_usage;
  this->bypp = 0; /* set a default value here */
  if (texture_format == PixelFormat_RGBA8888 ||
    texture_format == PixelFormat_BGRA8888) {
    this->bypp = 4;
  }
  else if (texture_format == PixelFormat_Float64) {
    this->bypp = 8;
  }
  else if (texture_format == PixelFormat_UInt8) {
    this->bypp = 1;
  }
  else {
    printf("Texture create failed: unsupported / unimplemented texture format.\n");
  }
  if (this->usage == TextureUsage_DepthBuffer) {
    if (this->format != PixelFormat_Float64) {
      printf("Texture create failed: depth buffer must have format float64.");
    }
  }
  this->pixels = malloc(w * h * bypp);
}

void Texture::load(const std::string & file, const PixelFormat & target_format, const TextureSampling& texture_sampling)
{
  int x, y, n;
  unsigned char *data = stbi_load(file.c_str(), &x, &y, &n, 4);
  if (data == NULL) {
    const char *failure = stbi_failure_reason();
    printf("Failed to load image \"%s\", %s.\n", file.c_str(), failure);
    printf("* note: current working directory is: \"%s\".\n", get_cwd().c_str());
    return;
  }
  this->create(x, y, PixelFormat_RGBA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  uint8_t *pixels = (uint8_t *)this->pixels;
  memcpy(pixels, data, x * y * 4);
  stbi_image_free(data);
  (*this) = this->to_format(target_format);
  this->set_sampling_mode(texture_sampling);
}

void Texture::clear(const Vec4& clear_color)
{
  if (this->usage == TextureUsage_DepthBuffer) {
    /* depth buffer is special, when it needs to be cleared,
    it should be set to 1.0, clear_color will be ignored. */
    int n_pixels = w * h;
    double *data = (double *)pixels;
    for (int i = 0; i < n_pixels; i++)
      data[i] = 1.0;
  }
  else if (this->format == PixelFormat_Float64) {
    /* if the texture format is float64 and it is not used as
    a depth buffer, we take the first component of clear_color
    and set all the pixels in the texture to this value. */
    int n_pixels = w * h;
    double *data = (double *)pixels;
    for (int i = 0; i < n_pixels; i++)
      data[i] = clear_color.i[0];
  }
  else if (this->format == PixelFormat_BGRA8888 || this->format == PixelFormat_RGBA8888) {
    uint8_t R, G, B, A;
    uint32_t packed_32bit;
    convert_Vec4_color_to_RGBA_uint8(clear_color, R, G, B, A);
    pack_RGBA8888_to_uint32(R, G, B, A, this->format, packed_32bit);
    int n_pixels = w * h;
    uint32_t *data = (uint32_t *)pixels;
    for (int i = 0; i < n_pixels; i++)
      data[i] = packed_32bit;
  }
  else if (this->format == PixelFormat_UInt8) {
    /* only the red component will be used */
    uint8_t R = uint8_t(clamp(0, int(clear_color.r * 255.0), 255));
    int n_pixels = w * h;
    uint8_t *data = (uint8_t *)pixels;
    for (int i = 0; i < n_pixels; i++)
      data[i] = R;
  }
  else {
    printf("Cannot clear texture, unsupported texture format or usage.\n");
  }
}

void Texture::copy(const Texture &texture) {
  this->create(texture.w, texture.h, texture.format, texture.sampling, texture.usage);
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

Vec4 Texture::texture_float64_point(const Vec2 & p) const
{
  /* point (nearest) sampling */
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);
  int x = min(int(p0.x * w), w - 1);
  int y = min(int(p0.y * h), h - 1);

  int pixel_id = y * w + x;
  double *data = (double *)pixels;

  return Vec4(data[pixel_id], 0.0, 0.0, 0.0);
}

Vec4 Texture::texture_xxxx8888_bilinear(const Vec2 & p) const
{
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);

  Vec4 output;
  sgl::bilinear_interpolation_xxxx8888((uint32_t*)this->pixels, this->format, this->w, this->h, p0, &output);
  return output;
}

Vec4 Texture::texture_float64_bilinear(const Vec2 & p) const
{
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);

  Vec4 output;
  sgl::bilinear_interpolation_scalar<double>((double*)this->pixels, this->w, this->h, p0, &output);
  return output;
}

Vec4 Texture::texture_uint8_point(const Vec2 & p) const
{
  /* point (nearest) sampling */
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);
  int x = min(int(p0.x * w), w - 1);
  int y = min(int(p0.y * h), h - 1);

  int pixel_id = y * w + x;
  uint8_t *data = (uint8_t *)pixels;
  uint8_t R = data[pixel_id];

  return Vec4(R, R, R, 1.0) / 255.0;
}

Vec4 Texture::texture_uint8_bilinear(const Vec2 & p) const
{
  Vec2 p0 = Vec2(p.x, 1.0 - p.y); /* flip ud */

  p0.x = max(min(p0.x, 1.0), 0.0);
  p0.y = max(min(p0.y, 1.0), 0.0);

  Vec4 output;
  sgl::bilinear_interpolation_scalar<uint8_t>((uint8_t*)this->pixels, this->w, this->h, p0, &output);
  return output;
}

Texture Texture::to_format(const PixelFormat & target_format) const
{
  if (this->format == target_format) {
    return (*this);
  }
  Texture converted_texture;
  converted_texture.create(this->w, this->h, target_format, this->sampling, this->usage);
  uint8_t* dst = (uint8_t*)converted_texture.pixels;
  uint8_t* src = (uint8_t*)this->pixels;
  if (this->format == PixelFormat_RGBA8888 && target_format == PixelFormat_BGRA8888) {
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
  else if (this->format == PixelFormat_BGRA8888 && target_format == PixelFormat_RGBA8888) {
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
  else if (this->format == PixelFormat_UInt8 && 
    (target_format == PixelFormat_RGBA8888 || target_format == PixelFormat_BGRA8888)) {
    for (int y = 0; y < this->h; y++) {
      for (int x = 0; x < this->w; x++) {
        int pid = y * this->w + x;
        dst[pid * 4 + 0] = src[pid];
        dst[pid * 4 + 1] = src[pid];
        dst[pid * 4 + 2] = src[pid];
        dst[pid * 4 + 3] = 255;
      }
    }
  }
  else if ((this->format == PixelFormat_RGBA8888 || this->format == PixelFormat_BGRA8888) && 
    target_format == PixelFormat_UInt8) {
    for (int y = 0; y < this->h; y++) {
      for (int x = 0; x < this->w; x++) {
        int pid = y * this->w + x;
        dst[pid] = src[pid * 4];
      }
    }
  }
  else {
    printf("Unimplemented texture format conversion type.\n");
    converted_texture.destroy();
  }
  return converted_texture;
}

bool Texture::save_png(const std::string & path) const
{
  if (this->w <= 0 || this->h <= 0 || this->pixels == NULL) {
    printf("Cannot save texture, texture object is invalid.\n");
    return false;
  }
  if (this->bypp != 4 || this->format == PixelFormat_Float64 || 
    this->format == PixelFormat_Unknown) {
    printf("Cannot save texture, unsupported pixel format.\n");
    return false;
  }
  /* stb image default to RGBA format */
  if (this->format != PixelFormat_RGBA8888) {
    Texture texobj = this->to_format(PixelFormat_RGBA8888);
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

Texture create_texture(int32_t w, int32_t h, PixelFormat format, TextureSampling sampling, TextureUsage usage)
{
  Texture texture;
  texture.create(w, h, format, sampling, usage);
  return texture;
}

Texture load_texture(const std::string &file, const PixelFormat& target_format, const TextureSampling& texture_sampling) {
  Texture texture;
  texture.load(file, target_format, texture_sampling);
  return texture;
}

Vec4 texture(const Texture *texobj, const Vec2 &uv) {
  if (texobj->sampling == TextureSampling_Bilinear) {
    if (texobj->format == PixelFormat_RGBA8888 || 
      texobj->format == PixelFormat_BGRA8888) {
      return texobj->texture_xxxx8888_bilinear(uv);
    }
    else if (texobj->format == PixelFormat_Float64) {
      return texobj->texture_float64_bilinear(uv);
    }
    else if (texobj->format == PixelFormat_UInt8) {
      return texobj->texture_uint8_bilinear(uv);
    }
  }
  else if (texobj->sampling == TextureSampling_Nearest) {
    if (texobj->format == PixelFormat_RGBA8888) {
      return texobj->texture_RGBA8888_point(uv);
    }
    else if (texobj->format == PixelFormat_BGRA8888) {
      return texobj->texture_BGRA8888_point(uv);
    }
    else if (texobj->format == PixelFormat_Float64) {
      return texobj->texture_float64_point(uv);
    }
    else if (texobj->format == PixelFormat_UInt8) {
      return texobj->texture_uint8_point(uv);
    }
  }
  return Vec4(0, 0, 0, 0);
}

void blit_texture(sgl::Texture * source, sgl::Texture * target,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y,
  sgl::Texture * src_mask)
{
  if (src_mask != NULL && src_mask->get_pixel_format() != PixelFormat_UInt8) {
    printf("blit mask texture must have pixel format uint8_t.\n");
    return;
  }
  if (source->get_pixel_format() != target->get_pixel_format()) {
    sgl::Texture tex = source->to_format(target->get_pixel_format());
    if (tex.get_pixel_data() == NULL)
      return;
    return blit_texture(&tex, target, src_x, src_y, src_w, src_h, dst_x, dst_y, src_mask);
  }

  /* blit operation starts here */
  int offx = dst_x - src_x, offy = dst_y - src_y;
  uint8_t* mask = src_mask ? (uint8_t*)src_mask->get_pixel_data() : NULL;

  /* 1 byte copy */
  if (source->get_bytes_per_pixel() == 1) {
    uint8_t* source_ptr = (uint8_t*)source->get_pixel_data();
    uint8_t* target_ptr = (uint8_t*)target->get_pixel_data();
    for (int sy = src_y; sy < src_y + src_h; sy++) {
      for (int sx = src_x; sx < src_x + src_w; sx++) {
        int dx = sx + offx, dy = sy + offy;
        bool src_valid = (sx >= 0 && sx < source->get_width() && sy >= 0 && sy < source->get_height());
        bool dst_valid = (dx >= 0 && dx < target->get_width() && dy >= 0 && dy < target->get_height());
        bool allow_copy = (mask == NULL || mask[sy * source->get_width() + sx] != 0);
        if (src_valid && dst_valid && allow_copy) {
          target_ptr[dy * target->get_width() + dx] = source_ptr[sy * source->get_width() + sx];
        }
      }
    }
  }
  else if (source->get_bytes_per_pixel() == 4) {
    uint32_t* source_ptr = (uint32_t*)source->get_pixel_data();
    uint32_t* target_ptr = (uint32_t*)target->get_pixel_data();
    for (int sy = src_y; sy < src_y + src_h; sy++) {
      for (int sx = src_x; sx < src_x + src_w; sx++) {
        int dx = sx + offx, dy = sy + offy;
        bool src_valid = (sx >= 0 && sx < source->get_width() && sy >= 0 && sy < source->get_height());
        bool dst_valid = (dx >= 0 && dx < target->get_width() && dy >= 0 && dy < target->get_height());
        bool allow_copy = (mask == NULL || mask[sy * source->get_width() + sx] != 0);
        if (src_valid && dst_valid && allow_copy) {
          target_ptr[dy * target->get_width() + dx] = source_ptr[sy * source->get_width() + sx];
        }
      }
    }
  }
  else if (source->get_bytes_per_pixel() == 8) {
    uint64_t* source_ptr = (uint64_t*)source->get_pixel_data();
    uint64_t* target_ptr = (uint64_t*)target->get_pixel_data();
    for (int sy = src_y; sy < src_y + src_h; sy++) {
      for (int sx = src_x; sx < src_x + src_w; sx++) {
        int dx = sx + offx, dy = sy + offy;
        bool src_valid = (sx >= 0 && sx < source->get_width() && sy >= 0 && sy < source->get_height());
        bool dst_valid = (dx >= 0 && dx < target->get_width() && dy >= 0 && dy < target->get_height());
        bool allow_copy = (mask == NULL || mask[sy * source->get_width() + sx] != 0);
        if (src_valid && dst_valid && allow_copy) {
          target_ptr[dy * target->get_width() + dx] = source_ptr[sy * source->get_width() + sx];
        }
      }
    }
  }
  else {
    printf("Unsupported bytes per pixel when trying to blit texture.\n");
  }
}

}; /* namespace sgl */
