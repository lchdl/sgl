/* Handling interactions between sgl and SDL2. */

#pragma once

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
sgl_texture_to_SDL_surface(const Texture* texture, SDL_Surface* surface);

};
};
