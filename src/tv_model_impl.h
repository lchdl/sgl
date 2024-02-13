#pragma once
#include <vector>
#include "tv_model.h"
#include "tv_math.h"
#include "tv_types.h"

struct tv_vertex {
  tv_vec3 p; /* position */
  tv_vec3 n; /* normal */
  tv_vec2 t; /* texture coordinate */
};

struct tv_mesh {
  /* static mesh object */
  std::vector<tv_vertex> v_buf; /* vertex buffer */
  std::vector<tv_u32_t> i_buf;  /* triangle vertex indices */
};


/*
defines how to interpolate two vertices `v0` and `v1` given mix weight `w`,
returns the interpolated vertex.
NOTE: `w` is a floating point number between -1 and +1,
      if `w`==0, function should return exactly `v0`,
      if `w`==1, function should return exactly `v1`.
*/
inline tv_vertex tv_lerp(const tv_vertex &v0, const tv_vertex &v1, const tv_float &w) 
{
  tv_vertex v;
  v.p = (tv_float(1.0) - w) * v0.p + tv_float(w) * v1.p;
  v.n = (tv_float(1.0) - w) * v0.n + tv_float(w) * v1.n;
  v.t = (tv_float(1.0) - w) * v0.t + tv_float(w) * v1.t;
  v.n = tv_normalize(v.n);
  return v;
}

