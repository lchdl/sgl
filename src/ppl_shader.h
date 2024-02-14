#pragma once

#include "ppl_math.h"

namespace ppl {

class Vertex {
public:
  Vec3 p; /* vertex position */
  Vec3 n; /* vertex normal */
  Vec2 t; /* vertex texture coordinate */
};

class Triangle {
public:
  Vertex v[3];
};

class Vertex_gl : public Vertex {
public:
  Vec4 gl_Position;
  /**
  Used in primitive clipping. Linear interpolate two vertices.
  @note: `gl_Position` should be lerped but `gl_FragCoord` does not need to.
  **/
  static Vertex_gl lerp(const Vertex_gl &v0, const Vertex_gl &v1,
                        const double &w) {
    return v0 * (1.0 - w) + v1 * w;
  }
  /**
  In rasterization, vertex attributes need to be first divided by real depth
  value, then interpolate by window space barycentric coordinates, finally
  multiply real depth again. So we need to provide such operators.
  @param v: Input vertex.
  @returns: Returns operated value.
  **/
  Vertex_gl operator+(const Vertex_gl &v) const {
    Vertex_gl v_out;
    v_out.p           = p + v.p;
    v_out.n           = n + v.n;
    v_out.t           = t + v.t;
    v_out.gl_Position = gl_Position + v.gl_Position;
    return v_out;
  }
  Vertex_gl operator*(const double &w) const {
    Vertex_gl v_out;
    v_out.p           = p * w;
    v_out.n           = n * w;
    v_out.t           = t * w;
    v_out.gl_Position = gl_Position * w;
    return v_out;
  }
  void operator*=(const double &w) {
    p *= w, n *= w, t *= w;
    gl_Position *= w;
  }
};

class Triangle_gl {
public:
  Vertex_gl v[3];

public:
  Triangle_gl() {}
  Triangle_gl(const Vertex_gl &v1, const Vertex_gl &v2, const Vertex_gl &v3) {
    this->v[0] = v1;
    this->v[1] = v2;
    this->v[2] = v3;
  }
  virtual ~Triangle_gl() {}
};

class Fragment_gl {
  /* data structure used by fragment shaders */
public:
  Vec4 gl_FragCoord;
  Vec3 n;
  Vec2 t;
};

class Uniforms {
public:
  Mat4x4 model;
  Mat4x4 view;
  Mat4x4 projection;
};

/**
Transform vertices from model local space to clip space.
**/
void vertex_shader(const Vertex &vertex_in, const Uniforms &uniforms,
                   Vertex_gl &vertex_out);

void fragment_shader(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &fragment_out);

};   // namespace ppl