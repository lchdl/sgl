/**
             ** SGL - Software Graphics Library **
---------------------------------------------------------------
                                By lchdl (chenghao1652@126.com)
                           Project started from Feb-2024 to now
A complete software implementation of OpenGL graphic pipeline.
This implementation also covers every details you need to know
about writing a software rasterizer from scratch. The whole
pipeline also supports OpenMP accelerating, you can dynamically
adjust the number of CPU cores used for rendering.
**/

#pragma once

/* core features */
#include "sgl_utils.h"
#include "sgl_math.h"
#include "sgl_enums.h"
#include "sgl_SDL2.h"
#include "sgl_texture.h"
#include "sgl_shader.h"
#include "sgl_model.h"
#include "sgl_pipeline.h"
#include "sgl_pass.h"

/* extension */
#include "sgl_primitives.h" /* draw 2D shapes or make 3D meshes */

/* audio */
#include "sgl_audio.h"

/* physics */
#include "sgl_physics/sgl_physics.h"

/* optional additional functions */
#ifdef ENABLE_OPENGL
#include "sgl_OpenGL.h"
#endif

/*
namespaces:

sgl         * Core software rasterization feature.
|             NOTE: this does not rely on any external libraries.
+--SDL2     * Implement communications between sgl and SDL2.
|             For example, SGL texture to SDL surface conversion.
+--[OpenGL] * Implements hardware acceleration by wrapping the 
|             OpenGL API using SDL2 and GLEW.
+--Audio    * Audio support using miniaudio.
+--Physics  * Simple physics support (TBD).

'[...]': Optional

*/
