#pragma once
#include "ppl_math.h"
#include <stdio.h>
#include <string>

namespace ppl {

inline void
print(const std::string &prefix, const Vec2 &v) {
  printf("%s (%.4f, %.4f)\n", prefix.c_str(), v.x, v.y);
};
inline void
print(const std::string &prefix, const Vec3 &v) {
  printf("%s (%.4f, %.4f, %.4f)\n", prefix.c_str(), v.x, v.y, v.z);
};
inline void
print(const std::string &prefix, const Vec4 &v) {
  printf("%s (%.4f, %.4f, %.4f, %.4f)\n", prefix.c_str(), v.x, v.y, v.z, v.w);
};
inline void
print(const std::string &prefix, const Vertex_gl &v) {
  printf("%s\n", prefix.c_str());
  printf("  gl_Position - (%.4f, %.4f, %.4f, %.4f)\n", v.gl_Position.x,
         v.gl_Position.y, v.gl_Position.z, v.gl_Position.w);
  printf("     position - (%.4f, %.4f, %.4f)\n", v.p.x, v.p.y, v.p.z);
  printf("       normal - (%.4f, %.4f, %.4f)\n", v.n.x, v.n.y, v.n.z);
  printf("      texture - (%.4f, %.4f)\n", v.t.x, v.t.y);
}

};   // namespace ppl
