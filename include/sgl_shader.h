#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"
#include <vector>

namespace sgl {

/* A vertex shader can only accept 8 input textures at maximum. */
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;
/* A vertex can only be affected by no more than 4 bones.
 * NOTE: this value cannot be changed. */
const int MAX_BONES_INFLUENCE_PER_VERTEX = 4; 
/* A mesh model can only have less than 128 nodes. */
const int MAX_NODES_PER_MODEL = 128;

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
  void operator/=(const double &w) {
    double t = 1.0 / w;
    return this->operator*=(t);
  }
};

struct Fragment_gl {
  /* fs_in */
  Vec4 gl_FragCoord;
  Vec3 wp;
  Vec3 wn;
  Vec2 t;
};

/* Uniform variables that are used by both vertex and fragment shaders. */
struct Uniforms {
  /* internal variables */
  Vec3 gl_DepthRange; /* (x=near, y=far, z=diff=far-near) */
  /* transforming vertex from local model space to world space. */
  Mat4x4 model;
  /* transforming vertex from world space to local view space. */
  Mat4x4 view;
  /* transforming vertex from local view space to homogeneous clip space. */
  Mat4x4 projection;
  /* texture objects */
  const Texture *in_textures[MAX_TEXTURES_PER_SHADING_UNIT];
  /* final bone transformations */
  Mat4x4 bone_matrices[MAX_NODES_PER_MODEL];

};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * Default shader implementations  * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/** 
Defines vertex and fragment shader function pointer types.
This will enable users to design their own vertex and fragment shaders
and link them to the pipeline.
**/
typedef void(*VS_func_t)(const Uniforms&, const Vertex&, Vertex_gl&);
typedef void(*FS_func_t)(const Uniforms&, const Fragment_gl&, Vec4&, bool&, double&);

/**
Defines default vertex shader (VS), which transforms vertices from model local 
space to homogeneous clip space. This function can also be used as a template.
  @param vertex_in: The input vertex.
  @param uniforms: Uniform variables used in vertex shader.
  @param vertex_out: The output vertex.
  @note: `gl_Position` of the @param vertex_out must be properly set.
**/
void default_VS(const Uniforms &uniforms, const Vertex &vertex_in, Vertex_gl &vertex_out);
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
This function can also be used as a template.
  @param fragment_in: The input fragment.
  @param uniforms: The input uniform variables.
  @param color_out: The calculated output color (in normalized range [0, 1]).
  @param discard: Whether this pixel is discarded or not.
**/
void default_FS(const Uniforms &uniforms, const Fragment_gl &fragment_in, Vec4 &color_out,
  bool& is_discarded, double& gl_FragDepth);

}; /* namespace sgl */
