/* defines the software rasterizer pipeline */
#pragma once
#include "tv_math.h"
#include "tv_model.h"
#include "tv_context.h"

void tv_pipeline_init(int n_ppls);
void tv_draw_mesh(tv_context* context, tv_mesh* mesh, const tv_mat4x4& world);
