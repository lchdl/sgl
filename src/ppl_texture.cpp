#include "ppl_texture.h"
#include <malloc.h>

namespace ppl {

Surface::Surface() {
  w = h  = 0;
  pixels = NULL;
}
Surface::~Surface() { destroy(); }
void
Surface::create(int32_t w, int32_t h, int32_t bypp) {
  this->w      = w;
  this->h      = h;
  this->bypp   = bypp;
  this->pixels = malloc(w * h * bypp);
}
void
Surface::destroy() {
  this->w = 0;
  this->h = 0;
  if (this->pixels)
    free(this->pixels);
  this->pixels = NULL;
}

};   // namespace ppl
