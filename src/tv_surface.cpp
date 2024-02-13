#include "tv_surface.h"

tv_surface *tv_surface_create(int w, int h, int pixbytes)
{ 
  tv_surface* surf = new tv_surface();
  surf->w = w;
  surf->h = h;
  surf->pixels = malloc(w*h*pixbytes);
  return surf;
}
void tv_surface_destroy(tv_surface *surface)
{
  if (surface){
    free(surface->pixels);
    delete surface;
  }
}
