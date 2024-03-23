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
#else /* Linux assumed */
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#endif

#include <stdio.h>
#include <vector>
#include <string>
#include <filesystem>

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
#else /* LINUX assumed */
  return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

inline std::string
get_cwd() {
	std::filesystem::path pwd = std::filesystem::current_path();
	return std::string(pwd);
}

inline void
set_cwd(const std::string& path) {
  std::filesystem::current_path(path);
}

inline bool 
endswith(std::string const &str, 
	std::string const &ending) {
	if (str.length() >= ending.length()) {
		return (0 == str.compare(str.length() - ending.length(), 
			ending.length(), ending));
	}
	else 
		return false;
}


inline bool 
file_exists(const std::string& file) {
	if (FILE *fobj = fopen(file.c_str(), "r")) {
		fclose(fobj);
		return true;
	}
	else
		return false;
}

/**
List all files in a folder (no recursive).
**/
inline std::vector<std::string>
ls(const std::string& folder) {
	std::vector<std::string> files;
	for (const auto& entry : std::filesystem::directory_iterator(folder)) {
		std::string file = entry.path().string();
		files.push_back(file);
	}
	return files;
}

/**
Remove folder.
**/
inline void
rm(const std::string& folder) {
	std::filesystem::remove_all(folder);
}

/**
Join path, returns absolute path.
**/
inline std::string
join(const std::string& path1, const std::string& path2) {
	std::filesystem::path joined = std::filesystem::path(path1) / std::filesystem::path(path2);
	return std::filesystem::absolute(joined).string();
}

/**
Make a directory, returns absolute path.
**/
inline std::string
mkdir(const std::string& folder) {
	std::filesystem::create_directories(folder);
	return std::filesystem::absolute(
		std::filesystem::path(folder)).string();
}

/**
Make a random folder in folder.
**/
inline std::string 
mktdir(const std::string& folder) {
	char dname[16];
	memset(dname, 0, 16);
	const char* choices =
		"0123456789"
		"abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t clen = strlen(choices);
	do {
		for (int i=0; i < 8; i++) {
			dname[i] = choices[rand() % clen];
		}
	} while (file_exists(join(folder, dname)));
	return mkdir(join(folder, dname));
}

/**
Get file directory.
**/
inline std::string
gd(const std::string& file) {
	std::filesystem::path ppath = std::filesystem::path(file).parent_path();
	return ppath.string();
}

/**
Replace a substring to another substring (in-place).
**/
inline void 
replace_all(std::string& str, 
    const std::string& from, 
    const std::string& to) {  
  if(from.empty())
    return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) 
        != std::string::npos) 
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

/**
Fast uint32_t memory buffer fill from:
**/
inline void
fill_u32(uint32_t* src, uint32_t n_elem, uint32_t val) {

}


};   // namespace sgl
