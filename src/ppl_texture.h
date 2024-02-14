#pragma once
#include <stdint.h>

namespace ppl {
class Surface {
public:
  int32_t w, h, bypp;
  void *pixels;

public:
  /**
  @param bypp: Bytes per pixel.
  **/
  void create(int32_t w, int32_t h, int32_t bypp);
  void destroy();

public:
  Surface();
  ~Surface();
};

};   // namespace ppl
