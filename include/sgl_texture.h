#pragma once
#include <stdint.h>

#include <map>
#include <string>

#include "sgl_math.h"

namespace sgl {

enum TextureFormat {
  texture_format_invalid,
  texture_format_RGBA8,
  texture_format_float64,
};

enum TextureSampling {
  texture_sampling_point,
};

class Texture {
 public:
  int32_t w, h, bypp;
  void *pixels;
  TextureFormat format;
  TextureSampling sampling;

 public:
  /**
  Create an empty texture.
  @param w, h: Width and height of the texture (in pixels).
  @param texture_format: Format of the created texture.
  @param texture_sampling: Defines how to interpolate texture data.
  **/
  void create(int32_t w, int32_t h, TextureFormat texture_format,
              TextureSampling texture_sampling);
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
  Vec4 texture_RGBA8_point(const Vec2 &p) const;

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

/**
  Common interface for sampling a texture. Designed mainly for fragment shaders.
  @param texobj: The texture object to be sampled.
  @param uv: Normalized texture coordinate. Out of bound values will be clipped.
  @returns: The sampled texture data returned as Vec4.
**/
Vec4 texture(const Texture *texobj, const Vec2 &uv);

class TextureLibrary {
 public:
  /**
  Load texture from disk and return texture handle.
  @param file: Image file path.
  @returns: The resource handle of loaded texture.
  **/
  int load_texture(const std::string &file);
  /**
  Get texture resource using texture handle.
  **/
  Texture *get_texture(int texture_handle);
  /**
  Create an empty texture.
  @returns: The resource handle of created texture.
  **/
  int create_texture(int w, int h, TextureFormat texture_format,
                     TextureSampling texture_sampling);
  /**
  Remove an existing texture.
  **/
  void remove_texture(int texture_handle);

 protected:
  int find_unused_handle();

 protected:
  std::map<int, Texture> texture_library;
};

};   // namespace sgl
