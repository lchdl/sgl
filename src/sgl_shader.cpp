#include "sgl_shader.h"

namespace sgl {

void assemble_fragment(const Vertex_gl & vertex_in, Fragment_gl & fragment_out) {
  fragment_out.wn = vertex_in.wn;
  fragment_out.wp = vertex_in.wp;
  fragment_out.t = vertex_in.t;
}

void FS_Outputs::reset()
{
  /* lazy reset */
  memset(this->set_flags, 0, sizeof(uint8_t)*MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS);
}

void FS_Outputs::set(const int & slot, const Vec4 & value)
{
  set_flags[slot]=1;
  out_comps[slot] = value;
}

Vec4 FS_Outputs::get(const int & slot) const
{
  return out_comps[slot];
}

uint8_t FS_Outputs::query(const int & slot) const
{
  return set_flags[slot];
}

void FS_Outputs::invalidate(const int & slot)
{
  set_flags[slot]=0;
}

FS_Outputs::FS_Outputs()
{
  reset();
}


}; /* namespace sgl */
