#include "sgl_SDL.h"

namespace sgl {
void 
RGBA8_texture_to_SDL_surface(const Texture * texture, SDL_Surface * surface) {
	uint8_t* src = (uint8_t*)texture->pixels;
	uint8_t* dst = (uint8_t*)surface->pixels;
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


};

