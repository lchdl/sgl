/* defines the software rasterizer pipeline */
#pragma once
#include "tv_math.h"
#include "tv_model.h"
#include "tv_context.h"

void tv_draw_mesh(tv_context* context, tv_mesh* mesh, tv_mat4x4* model);
