/**
             ** SGL - Software Graphics Library **
---------------------------------------------------------------
                                By lchdl (chenghao1652@126.com)
                           Project started from Feb-2024 to now
 -> "I enjoy implementing simple math principles behind complex
     (or seemingly complex) things using code."
A complete software implementation of OpenGL graphic pipeline.
This implementation also covers every details you need to know
about writing a software rasterizer from scratch. The whole
pipeline also supports OpenMP accelerating, you can dynamically
adjust the number of CPU cores used for rendering.
**/

#pragma once

#include "sgl_utils.h"
#include "sgl_SDL2.h"
#include "sgl_math.h"
#include "sgl_texture.h"
#include "sgl_shader.h"
#include "sgl_model.h"
#include "sgl_pipeline.h"
#include "sgl_pipeline_wireframe.h"
#include "sgl_pass.h"
