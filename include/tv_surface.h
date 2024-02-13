#pragma once
#include "tv_types.h"

struct tv_surface {
    /* 
    Render surface with arbitrary pixel formats,
    with data stored on RAM and can be randomly
    accessed by CPU. Surface object can also store
    loaded image texture data. 
    */
    tv_i32_t h, w;     /* size of the surface */
    void* pixels;  /* pixel data pointer */
};

tv_surface* tv_surface_create(int w, int h, int pixbytes);
void tv_surface_destroy(tv_surface* surface);



