#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"
#include <vector>

namespace sgl {

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
class Vertex_ppl {
  friend class Pipeline;
protected:
  /* this variable can be only accessed by Pipeline object, it 
  is not visible to the user */
  Vec4 gl_Position;
};
typedef class Vertex_gl : public Vertex_ppl {
public:
  /* vs_out & fs_in */
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
} Fragment_gl;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * Vertex and Fragment Shaders * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* Maximum fragment shader output color components */
const int MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS = 8;

/** 
Defines vertex and fragment shader function pointer types.
This will enable users to design their own vertex and fragment shaders
and link them to the pipeline.
**/
typedef void(*VS_func_t)(
  /* IN: uniform variables raw data pointer (can be casted to any user-defined uniform structs in custom vertex shader) */
  const void* uniforms_data,
  /* IN: input vertex */
  const Vertex& vertex_in,
  /* OUT: output vertex */
  Vertex_gl& vertex_out,
  /* OUT: `gl_Position` (must be properly set in a vertex shader) */
  Vec4& gl_Position
);

/**
A fragment shader can have multiple output components, and each 
component will write to its corresponding bound texture.
**/
class FS_Outputs {
  friend class Pipeline;
protected:
  Vec4 out_comps[MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS];
public:
  /* quick setter & getter */
  Vec4& operator[](const int& slot) { return this->out_comps[slot]; }
  const Vec4& operator[](const int& slot) const { return this->out_comps[slot]; };
  FS_Outputs() {}
};
typedef void(*FS_func_t)(
  /* IN: uniform variables raw data pointer (can be casted to any user-defined uniform structs in custom vertex shader) */
  const void* uniforms_data,
  /* IN: input fragment (which is also the output of the fragment shader, they are actually the same) */
  const Fragment_gl& fragment_in,
  /* IN: input fragment shader coordinates (`gl_FragCoord`) */
  const Vec4& gl_FragCoord,
  /* OUT: output fragment components (fragment shader can write to multiple target textures at the same time) */
  FS_Outputs& fragment_outs, 
  /* OUT: `discard`, if this fragment is discarded */
  bool& discard,
  /* OUT: `gl_FragDepth`, this value will be written to the z-buffer */
  double& gl_FragDepth
);


}; /* namespace sgl */
