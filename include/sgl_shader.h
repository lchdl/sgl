#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"
#include <vector>

namespace sgl {

class IVertex {
  template <typename U, typename V, typename F, typename S> 
  friend class Pipeline;
};

class IFragment {
  template <typename U, typename V, typename F, typename S> 
  friend class Pipeline;
protected:
  Vec4 gl_Position;
};

/* A vertex/fragment shader can only accept 8 input textures at maximum. */
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;

/* Maximum fragment shader output color components */
const int MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS = 8;

/**
A fragment shader can have multiple output components, and each
component will write to its own corresponding bound texture.
**/
class FS_Outputs {
  template <typename U, typename V, typename F, typename S> 
  friend class Pipeline;
protected:
  Vec4 out_comps[MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS];
public:
  Vec4& operator[](const int& slot) { return this->out_comps[slot]; }
  const Vec4& operator[](const int& slot) const { return this->out_comps[slot]; };
  FS_Outputs() {}
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* here we pre-define some common vertex formats for further use */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

struct Vertex_pnt : public IVertex {
  /*
  a basic vertex format with position (3), normal (3), and
  texture coordinates (2).
  */
  Vec3 p; /* vertex position (in model local space) */
  Vec3 n; /* vertex normal (in model local space)*/
  Vec2 t; /* vertex texture coordinate */
};
struct Vertex_pnt_bone : public IVertex {
  Vec3 position; /* vertex position (in model local space) */
  Vec3 normal; /* vertex normal (in model local space)*/
  Vec2 texcoord; /* vertex texture coordinate */
  /* normal mapping: tangent vector in tangent space */
  Vec3 tangent;
  /* for skeletal animations */
  IVec4 bone_IDs; /* bones up to 4 */
  Vec4  bone_weights;
};


}; /* namespace sgl */
