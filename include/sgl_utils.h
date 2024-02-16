#pragma once

#ifdef WIN32
#include <windows.h>
/* windows header file defined lots of dirty stuff... */
#undef near
#undef far
#undef max
#undef min
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <string>

#include "sgl_math.h"

namespace sgl {

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

#if defined(WINDOWS) || defined(WIN32)
class Timer {
public:
	double tick() {
		QueryPerformanceCounter(&tnow);
		double dt = double(tnow.QuadPart - tlast.QuadPart) / double(frequency.QuadPart);
		tlast = tnow;
		return dt;
	}
	double elapsed() {
		QueryPerformanceCounter(&tnow);
		double dt = double(tnow.QuadPart - tlast.QuadPart) / double(frequency.QuadPart);
		return dt;
	}
	Timer() { 
		QueryPerformanceFrequency(&frequency); 
		tick(); 
	}

protected:
	LARGE_INTEGER frequency;
	LARGE_INTEGER tlast, tnow;
};
#elif defined (LINUX)
class Timer {
public:
	double tick() {
		gettimeofday(&tnow, NULL);
		double dt = (tnow.tv_sec - tlast.tv_sec);
		dt += (tnow.tv_usec - tlast.tv_usec) / 1000000.0; /* us to s */
		tlast = tnow;
		return dt;
	}
	double elapsed() {
		gettimeofday(&tnow, NULL);
		double dt = (tnow.tv_sec - tlast.tv_sec);
		dt += (tnow.tv_usec - tlast.tv_usec) / 1000000.0; /* us to s */
		return dt;
	}
	Timer() { tick(); }

protected:
	timeval tlast, tnow;
};
#endif


inline int
get_cpu_cores() {
#ifdef WIN32
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return sysinfo.dwNumberOfProcessors;
#elif MACOS
  int nm[2];
  size_t len = 4;
  uint32_t count;

  nm[0] = CTL_HW;
  nm[1] = HW_AVAILCPU;
  sysctl(nm, 2, &count, &len, NULL, 0);

  if (count < 1) {
    nm[1] = HW_NCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
    if (count < 1) {
      count = 1;
    }
  }
  return count;
#else
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

inline std::string
get_cwd() {
	TCHAR pwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, pwd);
	return std::string(pwd);
}

};   // namespace sgl
