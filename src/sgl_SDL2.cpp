#include "sgl_SDL2.h"

namespace sgl {
namespace SDL2 {

void
sgl_texture_to_SDL2_surface(const Texture * texture, SDL_Surface * surface) {
  /*
  Many graphics drivers and graphics APIs use BGRA8888 as their default surface format.
  Since SDL2 use Direct3D or OpenGL as its backend, it also follows the same native surface format
  as Direct3D or OpenGL.
  */
  uint8_t* src = (uint8_t*)texture->pixels;
  uint8_t* dst = (uint8_t*)surface->pixels;
  const uint32_t buffer_bytes = 4 * texture->w * texture->h;
  if (texture->format == PixelFormat::pixel_format_RGBA8888) {
    for (int y = 0; y < surface->h; y++) {
      for (int x = 0; x < surface->w; x++) {
        int pid = y * texture->w + x;
        dst[pid * 4 + 2] = src[pid * 4 + 0];
        dst[pid * 4 + 1] = src[pid * 4 + 1];
        dst[pid * 4 + 0] = src[pid * 4 + 2];
        dst[pid * 4 + 3] = src[pid * 4 + 3];
      }
    }
  }
  else if (texture->format == PixelFormat::pixel_format_BGRA8888) {
    memcpy(dst, src, buffer_bytes);
  }
  else {
    /* other types of texture formats are currently not supported */
    printf("sgl_texture_to_SDL2_surface(): "
      "other types of texture formats are "
      "currently not supported.\n");
  }
}

};
};

