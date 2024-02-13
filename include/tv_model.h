#pragma once
#include "tv_math.h"

struct tv_mesh;

tv_mesh* tv_mesh_create();
void tv_mesh_destroy(tv_mesh* mesh);
int tv_mesh_add_vertex(tv_mesh* mesh, const tv_vec3& p, const tv_vec3& n, const tv_vec2& t); /* returns added vertex index */
int tv_mesh_add_triangle(tv_mesh* mesh, const int& i1, const int& i2, const int& i3);
