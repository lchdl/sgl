#pragma once
#include "tv_context.h"
#include "tv_types.h"
#include "tv_math.h"

struct tv_context {
    /* render context (camera, attached surface, ...) */
    tv_u32_t c_flag; /* 32-bit context global flags */
    /* 
    31           24 23           16 15            8 7             0
    V D C ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^
    -------------
    V: is valid (0/1: invalid/valid)
    D: debug context (0/1: disable/enable)
    C: backface culling (0/1: disable/enable)

    ^: unused
    */
    
    /* 
    NOTE: the following surface resources are not owned by context object.
    */
    tv_surface* color_surface;
    tv_surface* depth_surface;
    tv_surface* stencil_surface;

    /* camera parameters */
    tv_vec3 eye, look, up; 
    struct {
        tv_u8_t enabled;
        tv_float field_of_view; /* perspective mode (radians) */
        tv_float near, far;
    } perspective;
    struct {
        tv_u8_t enabled;
        /* if both perspective and orthographic mode are enabled, */
        /* default to perspective mode. */
        tv_float ortho_h, ortho_w; /* orthographic mode */
        tv_float near, far;
    } orthographic;

    tv_context();

};

#define TV_CONTEXT_VALID_BIT      0x80000000
#define TV_DEBUG_CONTEXT_BIT      0x40000000
#define TV_BACKFACE_CULLING_BIT   0x20000000
