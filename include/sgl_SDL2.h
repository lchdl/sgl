/* Handling interactions between sgl and SDL2. */

#pragma once

/*
SDL (Simple DirectMedia Layer)
* Project main page:
  https://wiki.libsdl.org/SDL3/FrontPage
* **Tutorial** by LazyFoo:
  https://lazyfoo.net/tutorials/SDL/
* Initialize OpenGL in SDL:
  https://raw.githubusercontent.com/Overv/Open.GL/master/ebook/Modern%20OpenGL%20Guide.pdf
*/
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "sgl_texture.h"

namespace sgl {
namespace SDL2 {

/**
Convert sgl texture object to SDL2 surface object.
  @note: Texture and surface should have the same size. For efficiency, this
  function will not check the sizes of texture and surface.
  Only support RGBA8 format.
**/
void
sgl_texture_to_SDL2_surface(const Texture* texture, SDL_Surface* surface);

};
};
