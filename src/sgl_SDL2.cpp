#include "sgl_SDL2.h"

namespace sgl {
namespace SDL2 {

void
sgl_texture_to_SDL_surface(const Texture * texture, SDL_Surface * surface) {
	if (texture->format == TextureFormat::texture_format_RGBA8) {
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
	else {
		/* other types of texture formats are currently not supported */
		printf("sgl_texture_to_SDL_surface(): "
			"other types of texture formats are "
			"currently not supported.\n");
	}
}

};
};

