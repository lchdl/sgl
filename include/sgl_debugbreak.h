#pragma once

#if defined(LINUX)
#include "signal.h"
inline void debugbreak() {
  raise(SIGTRAP);
}
#elif defined(WINDOWS) || defined(WIN32)
#include <intrin.h>
inline void debugbreak() {
  __debugbreak();
}
#endif
