/* communications between tiny-vision and SDL2 library */
#pragma once
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "tv_types.h"
#include "tv_surface.h"

/* convert tv surface to SDL surface for final display */
void tv_surface_to_SDL(tv_surface* src_surf, SDL_Surface* dst_surf);




