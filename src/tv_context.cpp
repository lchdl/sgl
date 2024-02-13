#include "tv_context_impl.h"
#include "tv_math.h"
#include <malloc.h>

tv_context::tv_context()
{
    c_flag = 0x80000000; /* set valid bit */
    c_flag |= 0x20000000; /* enable backface culling */
    color_surface = NULL;
    depth_surface = NULL;
    stencil_surface = NULL;
    /* default to perspective mode */
    orthographic.enabled = false;
    orthographic.ortho_h = 0;
    orthographic.ortho_w = 0;
    orthographic.near = tv_float(1.0);
    orthographic.far = tv_float(100);
    perspective.field_of_view = TV_PI / tv_float(3.0);
    perspective.enabled = true;
    perspective.near = tv_float(1.0);
    perspective.far = tv_float(100.0);
    eye = tv_vec3(5, 5, 5);
    look = tv_vec3(0, 0, 0);
    up = tv_vec3(0, 1, 0);
}
