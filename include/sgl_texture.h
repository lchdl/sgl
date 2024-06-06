#pragma once
#include <stdint.h>

#include <map>
#include <string>

#include "sgl_math.h"

namespace sgl {

enum PixelFormat {
  /*

  Defines the physical storage order in little-endian system.

  Useful reference:
  http://http.download.nvidia.com/developer/Papers/2005/Fast_Texture_Transfers/Fast_Texture_Transfers.pdf
  => "Storing 8-bit textures in a BGRA layout in system memory 
      and use theGL_BGRA as the external format for textures 
      to avoid swizzling."

  */
  pixel_format_unknown,
  pixel_format_RGBA8888,
  pixel_format_BGRA8888, /* NVIDIA graphics card native format */
  pixel_format_float64,
};

enum TextureSampling {
  texture_sampling_point,
};

class Texture {
 public:
  int32_t w, h, bypp;
  void *pixels;
  PixelFormat format;
  TextureSampling sampling;

 public:
  /**
  Create an empty texture.
  @param w, h: Width and height of the texture (in pixels).
  @param texture_format: Format of the created texture.
  @param texture_sampling: Defines how to interpolate texture data.
  **/
  void create(int32_t w, int32_t h, 
    PixelFormat texture_format = PixelFormat::pixel_format_BGRA8888,
    TextureSampling texture_sampling = TextureSampling::texture_sampling_point);
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
  Vec4 texture_RGBA8888_point(const Vec2 &p) const;
  Vec4 texture_BGRA8888_point(const Vec2 &p) const;
  /**
  Convert texture to another format.
  **/
  Texture to_format(const PixelFormat& target_format) const;

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
  Load an image from disk and return the loaded texture object.
  @param file: Image file path.
  @returns: The loaded image texture. If image loading failed, an empty texture
will be returned (pixels=NULL).
**/
Texture load_texture(const std::string &file, 
  const PixelFormat& target_format = PixelFormat::pixel_format_BGRA8888);

/**
  Common interface for sampling a texture. Designed mainly for fragment shaders.
  @param texobj: The texture object to be sampled.
  @param uv: Normalized texture coordinate. Out of bound values will be clipped.
  @returns: The sampled texture data returned as Vec4.
**/
Vec4 texture(const Texture *texobj, const Vec2 &uv);

}; /* namespace sgl */
