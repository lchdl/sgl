#pragma once

/*

Contains all enumerations used throughout the SGL library.

This header defines the complete set of enumeration types that 
are utilized across various components of the Software Graphics 
Library (SGL). These enums provide type-safe identifiers for 
graphics operations, states, and configurations.

*/

namespace sgl {

/* namespace sgl */
enum DeviceType {
  DeviceType_CPU,
  /* 
  If OpenGL is enabled, then `sgl::OpenGL` namespace will be 
  available, `DeviceType_GPU` is made specifially for this.
  */
  DeviceType_GPU, 
};
enum PixelFormat {
  /*

  Defines the physical storage order in little-endian system.

  Useful reference:
  http://http.download.nvidia.com/developer/Papers/2005/Fast_Texture_Transfers/Fast_Texture_Transfers.pdf
  => "Storing 8-bit textures in a BGRA layout in system memory
      and use the GL_BGRA as the external format for textures
      to avoid swizzling."

  */
  PixelFormat_Unknown,
  PixelFormat_RGBA8888,
  PixelFormat_BGRA8888, /* NVIDIA graphics card native format */
  PixelFormat_Float64, /* used in depth buffer, a single pixel stores a float64 */
  PixelFormat_Float32, /* avoid this format in software rasterizer as much as possible */
  PixelFormat_UInt8, /* often used as stencil buffers or other flag buffers. */
  /*

  Below are the OpenGL-specific pixel formats.
  NOTE: If a new OpenGL-specific format is added, ensure the following
  two functions are extended to support the new format:
    1. sgl::Texture::create();
    2. sgl::OpenGL::Texture::to_device();
    3. sgl::OpenGL::blit_texture(); <- for blitting this new type texture onto screen

  */
  PixelFormat_OpenGL_RG32F, /* each pixel stores two float32 */
};
enum TextureSampling {
  TextureSampling_Nearest, /* point (nearest) sampling */
  TextureSampling_Bilinear,
};
enum TextureUsage {
  TextureUsage_Unknown,
  TextureUsage_ColorComponents,
  TextureUsage_DepthBuffer,
};
enum PipelineDrawMode {
  PipelineDrawMode_Wireframe,
  PipelineDrawMode_Triangle,
};
enum KeyFrameInterpType {
  KeyFrameInterpType_Nearest,
  KeyFrameInterpType_Linear,
};
enum SpriteOriginMode {
  SpriteOriginMode_Center,      /* Origin is at the center of the sprite. */
  SpriteOriginMode_BottomLeft,  /* Origin is at the bottom-left corner of the sprite. */
  SpriteOriginMode_BottomRight, /* Origin is at the bottom-right corner of the sprite.  */
  SpriteOriginMode_TopLeft,     /* Origin is at the top-left corner of the sprite. */
  SpriteOriginMode_TopRight,    /* Origin is at the top-right corner of the sprite. */
};

#ifdef ENABLE_OPENGL
namespace OpenGL {
/* namespace sgl::OpenGL */
enum TextureWrapMode {
  TextureWrapMode_Repeat,
  TextureWrapMode_MirroredRepeat,
  TextureWrapMode_ClampToEdge,
  TextureWrapMode_ClampToBorder,
};
#endif

}; /* namespace sgl::OpenGL */
}; /* namespace sgl */
