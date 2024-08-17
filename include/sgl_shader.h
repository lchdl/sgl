#pragma once

#include "sgl_math.h"
#include "sgl_texture.h"
#include <vector>

namespace sgl {

class IVertex {
  template <typename U, typename V, typename F, typename S> 
  friend class Pipeline;
protected:
  Vec4 gl_Position;
};

class IFragment : public IVertex {
  template <typename U, typename V, typename F, typename S> 
  friend class Pipeline;
};

/* A vertex/fragment shader can only accept 8 input textures at maximum. */
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;

/* Maximum fragment shader output color components */
const int MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS = 8;

/**
A fragment shader can have multiple output components, and each
component will write to its corresponding bound texture.
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

}; /* namespace sgl */
