/* Handling interactions between sgl and SDL2. */

#pragma once

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "sgl_texture.h"

namespace sgl {

/**
Convert sgl texture object to SDL2 surface object.
	@note: texture and surface should have the same size.
**/
void
RGBA8_texture_to_SDL_surface(const Texture* texture, SDL_Surface* surface);

};