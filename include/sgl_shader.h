#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"

namespace sgl {

/* A vertex shader can only accept 8 input textures at maximum. */
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;
/* A vertex can only be affected by no more than 4 bones.
 * NOTE: this value cannot be changed. */
const int MAX_BONES_INFLUENCE_PER_VERTEX = 4; 
/* A mesh model can only have less than 128 bones. */
const int MAX_BONES_PER_MESH = 128;

class Vertex {
 public:
  /* static geometry */
  Vec3 p; /* vertex position (in model local space) */
  Vec3 n; /* vertex normal (in model local space)*/
  Vec2 t; /* vertex texture coordinate */
  /* bones & animations */
  IVec4 bone_IDs; /* bones up to 4 */
  Vec4  bone_weights;
};

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
  static Vertex_gl lerp(const Vertex_gl &v0, const Vertex_gl &v1,
                        const double &w) {
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
};

class Fragment_gl {
  /* data structure used by fragment shaders */
 public:
  /* fs_in */
  Vec4 gl_FragCoord;
  Vec3 wp;
  Vec3 wn;
  Vec2 t;
};

class Uniforms {
  /* Uniform variables that are used by both vertex and fragment shaders. */
 public:
  /* transforming vertex from local model space to world space. */
  Mat4x4 model;
  /* transforming vertex from world space to local view space. */
  Mat4x4 view;
  /* transforming vertex from local view space to homogeneous clip space. */
  Mat4x4 projection;
  /* texture objects */
  const Texture *in_textures[MAX_TEXTURES_PER_SHADING_UNIT];
  /* final bone transformation matrices array (from root to leaf) for a mesh */
  const Mat4x4* bone_matrices; /* [MAX_BONES_PER_MESH] */
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * Default shader implementations  * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** 
Defines vertex and fragment shader function pointer types.
This will enable users to design their own vertex and fragment shaders
and link them to the pipeline.
**/
typedef void(*VS_func_t)(const Vertex &, const Uniforms &, Vertex_gl &);
typedef void(*FS_func_t)(const Fragment_gl &, const Uniforms &, Vec4 &, bool&);

/**
Defines default vertex shader (VS), which transforms vertices from 
model local space to homogeneous clip space.
  @param vertex_in: The input vertex.
  @param uniforms: Uniform variables used in vertex shader.
  @param vertex_out: The output vertex.
  @note: `gl_Position` of the  @param vertex_out must be properly set.
**/
void VS_default(const Vertex &vertex_in, const Uniforms &uniforms,
                   Vertex_gl &vertex_out);
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

/**
Defines default fragment shader (FS), shades each fragment into color output.
  @param fragment_in: The input fragment.
  @param uniforms: The input uniform variables.
  @param color_out: The calculated output color (in normalized range [0, 1]).
	@param discard: Whether this pixel is discarded or not.
**/
void FS_default(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard);

};   // namespace sgl
