#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"

namespace sgl {

const int MAX_TEXTURES_PER_SHADING_UNIT = 8;

class Vertex {
 public:
  /* vs_in */
  Vec3 p; /* vertex position (in model local space) */
  Vec3 n; /* vertex normal (in model local space)*/
  Vec2 t; /* vertex texture coordinate */
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
};

/* Describes a complete pass when rendering */
struct Pass {
  /* output color texture (write only) */
  Texture* color_texture;
  /* output depth texture (write only) */
  Texture* depth_texture;
  /* input textures for vertex and fragment shaders (read only) */
  const Texture* in_textures[MAX_TEXTURES_PER_SHADING_UNIT];

  Mat4x4 model_transform;

  /* camera/eye settings */
  struct {
    Vec3 position; /* eye position */
    Vec3 look_at;  /* view target */
    Vec3 up_dir;   /* up normal */
    struct {
      bool enabled;
      double near, far, field_of_view;
    } perspective;
    struct {
      bool enabled;
      double width, height, depth;
    } orthographic;
  } eye;

 public:
  /* default ctor */
  Pass();
  /* utility functions */
  void to_uniforms(Uniforms& out_uniforms) const;
  Mat4x4 get_view_matrix() const;
  Mat4x4 get_projection_matrix() const;

};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**
Defines vertex shader (VS), which transforms vertices from model local space
to homogeneous clip space.
  @param vertex_in: The input vertex.
  @param uniforms: Uniform variables used in vertex shader.
  @param vertex_out: The output vertex.
  @note: `gl_Position` of the  @param vertex_out must be properly set.
**/
void vertex_shader(const Vertex &vertex_in, const Uniforms &uniforms,
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
Defines fragment shader (FS).
  @param fragment_in: The input fragment.
  @param uniforms: The input uniform variables.
  @param color_out: The calculated output color (in normalized range [0, 1]).
	@param discard: Whether this pixel is discarded or not.
**/
void fragment_shader(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard);

void fragment_shader2(const Fragment_gl & fragment_in, const Uniforms & uniforms, Vec4 & fragment_out, bool& discard);

};   // namespace sgl
