#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"
#include <vector>

namespace sgl {

/* A vertex/fragment shader can only accept 8 input textures at maximum. */
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;
/* Maximum fragment shader output color components */
const int MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS = 8;

struct Vertex {
  Vec3 p; /* vertex position (in model local space) */
  Vec3 n; /* vertex normal (in model local space)*/
  Vec2 t; /* vertex texture coordinate */
  /* for skeletal animations */
  IVec4 bone_IDs; /* bones up to 4 */
  Vec4  bone_weights;
};

typedef std::vector<Vertex> VertexBuffer_t;
typedef std::vector<int32_t> IndexBuffer_t;

/**
Internal vertex format used by the pipeline.
After vertex processing stage is finished, all vertices will be 
stored using this format. The vertex shader actually tells the 
pipeline how to convert raw `Vertex` to `Vertex_gl`.
@NOTE: vertex shader should at least fill the `gl_Position` member
properly, it will be used by the fragment shader in fragment 
processing stage.
**/
class Vertex_gl {
public:
  /* vs_out */
  Vec4 gl_Position;
  Vec3 wp; /* world position */
  Vec3 wn; /* world normal */
  Vec2 t;  /* texture coordinates */

public:
  /**
  Used in primitive clipping. Linear interpolate two vertices.
  @note: `gl_Position` should be lerped but `gl_FragCoord` does not need to, it
  will be automatically assembled in rasterization stage.
  **/
  static Vertex_gl lerp(const Vertex_gl &v0, const Vertex_gl &v1, const double &w) {
    return v0 * (1.0 - w) + v1 * w;
  }
  /**
  In rasterization, vertex attributes need to be first divided by real depth
  value, then interpolate by window space barycentric coordinates, finally
  multiply real depth again, to obtain the interpolated perspective correct
  attribute values. So we need to provide such operators.
  @param v: Input vertex.
  @returns: Returns operated value.
  **/
  Vertex_gl operator+(const Vertex_gl &v) const {
    Vertex_gl v_out;
    v_out.wp = wp + v.wp;
    v_out.wn = wn + v.wn;
    v_out.t = t + v.t;
    v_out.wp = wp + v.wp;
    v_out.gl_Position = gl_Position + v.gl_Position;
    return v_out;
  }
  Vertex_gl operator*(const double &w) const {
    Vertex_gl v_out;
    v_out.wp = wp * w;
    v_out.wn = wn * w;
    v_out.t = t * w;
    v_out.wp = wp * w;
    v_out.gl_Position = gl_Position * w;
    return v_out;
  }
  void operator*=(const double &w) {
    wp *= w, wn *= w, t *= w, wp *= w;
    gl_Position *= w;
  }
  void operator/=(const double &w) {
    double t = 1.0 / w;
    return this->operator*=(t);
  }
};

struct Fragment_gl {
  /* fs_in */
  Vec4 gl_FragCoord;
  Vec3 wp; /* world position */
  Vec3 wn; /* world normal */
  Vec2 t;  /* texture coordinates */
};
/**
Assemble fragment from interpolated vertex. The assembled fragment will be sent
to fragment shader immediately.
  @param vertex_in: The interpolated vertex generated in rasterization stage.
  @param fragment_out: The assembled output fragment. After assembling this
fragment will be sent into fragment_shader( @param fragment_in, ... ).
  @note: `gl_FragCoord` of the @param fragment_in does not need to be set by
users, as this member will be properly set by the rasterization pipeline.
**/
void assemble_fragment(const Vertex_gl &vertex_in, Fragment_gl &fragment_out);

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * Vertex and Fragment Shaders * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** 
Defines vertex and fragment shader function pointer types.
This will enable users to design their own vertex and fragment shaders
and link them to the pipeline.
**/
typedef void(*VS_func_t)(const void*, const Vertex&, Vertex_gl&);

/**
A fragment shader can have multiple output components, and each 
component will write to its corresponding bound texture.
**/
class FS_Outputs {
  Vec4 out_comps[MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS];
  /* set_flags: 0 = not used / invalid, 1 = set */
  uint8_t set_flags[MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS]; 
public:
  /* reset: all color components are invalidated */
  void reset();
  /* set a specific color components with customized value */
  void set(const int& slot, const Vec4& value);
  /* get color components in a specific slot, if the slot is not set (flag=0),
  then result is undefined. */
  Vec4 get(const int& slot) const;
  /* check if a color slot is used */
  uint8_t query(const int& slot) const;
  /* invalidate a slot */
  void invalidate(const int& slot);
  /* ctor */
  FS_Outputs();
  /* pure data struct/class like this does not need dtor */
};
typedef void(*FS_func_t)(const void*, const Fragment_gl&, FS_Outputs&, bool&, double&);

}; /* namespace sgl */
