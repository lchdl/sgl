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
#include <cctype>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <filesystem>
#include <fstream>
#include <streambuf>

#include "sgl_math.h"

#if defined(LINUX)
#include "signal.h"
inline void debugbreak(){
  raise(SIGTRAP);
}
#elif defined(WINDOWS) || defined(WIN32)
inline void debugbreak(){
  __debugbreak();
}
#endif

#define SGL_INCREMENT_IT_PTR(it)            (++(*it))
#define SGL_PTR_DIFF(it, out)               (size_t)(it - out)
#define SGL_STRING_OP_MASK8(oc)             ((uint8_t)(0xff & (oc)))
#define SGL_STRING_OP_MASK16(oc)            ((uint16_t)(0xffff & (oc)))
#define SGL_STRING_OP_LEAD_SURROGATE_MIN    0xd800u
#define SGL_STRING_OP_LEAD_SURROGATE_MAX    0xdbffu
#define SGL_STRING_OP_TRAIL_SURROGATE_MIN   0xdc00u
#define SGL_STRING_OP_LEAD_OFFSET           (SGL_STRING_OP_LEAD_SURROGATE_MIN - (0x10000 >> 10))
#define SGL_STRING_OP_IS_LEAD_SURROGATE(cp) ((cp) >= SGL_STRING_OP_LEAD_SURROGATE_MIN && (cp) <= SGL_STRING_OP_LEAD_SURROGATE_MAX)
#define SGL_STRING_OP_SURROGATE_OFFSET      (0x10000u - (SGL_STRING_OP_LEAD_SURROGATE_MIN << 10) - SGL_STRING_OP_TRAIL_SURROGATE_MIN)

namespace sgl {

inline void print(const Vec2 &v) {
  printf("(%.4f, %.4f)\n", v.x, v.y);
};
inline void print(const Vec3 &v) {
  printf("(%.4f, %.4f, %.4f)\n", v.x, v.y, v.z);
};
inline void print(const Vec4 &v) {
  printf("(%.4f, %.4f, %.4f, %.4f)\n", v.x, v.y, v.z, v.w);
};

inline void print(const std::string &prefix, const Vec2 &v) {
  printf("%s (%.4f, %.4f)\n", prefix.c_str(), v.x, v.y);
};
inline void print(const std::string &prefix, const Vec3 &v) {
  printf("%s (%.4f, %.4f, %.4f)\n", prefix.c_str(), v.x, v.y, v.z);
};
inline void print(const std::string &prefix, const Vec4 &v) {
  printf("%s (%.4f, %.4f, %.4f, %.4f)\n", prefix.c_str(), v.x, v.y, v.z, v.w);
};

inline void print(const Mat2x2& m) {
  printf("[ %.2lf %.2lf\n", m.i11, m.i12);
  printf("  %.2lf %.2lf ]\n", m.i21, m.i22);
}
inline void print(const Mat3x3& m) {
  printf("[ %.2lf %.2lf %.2lf\n", m.i11, m.i12, m.i13);
  printf("  %.2lf %.2lf %.2lf\n", m.i21, m.i22, m.i23);
  printf("  %.2lf %.2lf %.2lf ]\n", m.i31, m.i32, m.i33);
}
inline void print(const Mat4x4& m) {
  printf("[ %.2lf %.2lf %.2lf %.2lf\n", m.i11, m.i12, m.i13, m.i14);
  printf("  %.2lf %.2lf %.2lf %.2lf\n", m.i21, m.i22, m.i23, m.i24);
  printf("  %.2lf %.2lf %.2lf %.2lf\n", m.i31, m.i32, m.i33, m.i34);
  printf("  %.2lf %.2lf %.2lf %.2lf ]\n", m.i41, m.i42, m.i43, m.i44);
}

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
  return pwd.generic_string();
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
Get file name.
**/
inline std::string
gn(const std::string& file) {
  std::filesystem::path file_path = file;
  std::filesystem::path file_name = file_path.filename();
  return file_name.string();
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
inline void
replace_all(std::wstring& str,
  const std::wstring& from,
  const std::wstring& to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos))
    != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
}

/**
Split a standard string using a string delimiter.
Original answer from:
https://stackoverflow.com/questions/14265581/parse-split-a-string-in-c-using-string-delimiter-standard-c.
**/
inline std::vector<std::string> 
split(std::string& s, const std::string& delimiter) {
  std::vector<std::string> tokens;
  size_t pos = 0;
  std::string token;
  while ((pos = s.find(delimiter)) != std::string::npos) {
    token = s.substr(0, pos);
    tokens.push_back(token);
    s.erase(0, pos + delimiter.length());
  }
  tokens.push_back(s);
  return tokens;
}

/**
Reads a text file as a string.
Note: This function reads the entire file as a raw string 
without making any assumptions about its encoding.
**/
inline std::string
read_file_as_string(const char* file) {
  if (!file_exists(file)) {
    printf("File \"%s\" not exist.\n", file);
    return std::string("");
  }
  std::ifstream f(file);
  std::string s((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());
  return s;
}

/*
Code adapted from:
https://github.com/tapika/cutf/blob/master/cutf.cpp.

Converts utf-8 string to wide version.
If out == NULL, then estimate how much buffer space is needed.
Returns target string length. Length includes extra NUL ('\0')
terminating character.
*/
inline size_t 
_utf8_to_wchar(const char* s, size_t in_size, wchar_t* out, size_t out_size) {
  uint8_t* start = (uint8_t*)s;
  if (in_size == SIZE_MAX)
    in_size = strlen(s);

  uint8_t* end = start + in_size;

  auto sequence_length = [&](uint8_t* lead_it) -> size_t {
    uint8_t lead = SGL_STRING_OP_MASK8(*lead_it);
    if (lead < 0x80) return 1;
    else if ((lead >> 5) == 0x6) return 2;
    else if ((lead >> 4) == 0xe) return 3;
    else if ((lead >> 3) == 0x1e) return 4;
    return 0;
  };

  auto next = [&](uint8_t** it, size_t* remain) -> uint32_t {
    uint32_t cp = SGL_STRING_OP_MASK8(**it);
    size_t length = sequence_length(*it);

    if (remain)
      *remain -= length;

    switch (length) {
    case 1:
      break;
    case 2:
      SGL_INCREMENT_IT_PTR(it);
      cp = ((cp << 6) & 0x7ff) + ((**it) & 0x3f);
      break;
    case 3:
      SGL_INCREMENT_IT_PTR(it);
      cp = ((cp << 12) & 0xffff) + ((SGL_STRING_OP_MASK8(**it) << 6) & 0xfff);
      SGL_INCREMENT_IT_PTR(it);
      cp += (**it) & 0x3f;
      break;
    case 4:
      SGL_INCREMENT_IT_PTR(it);
      cp = ((cp << 18) & 0x1fffff) + ((SGL_STRING_OP_MASK8(**it) << 12) & 0x3ffff);
      SGL_INCREMENT_IT_PTR(it);
      cp += (SGL_STRING_OP_MASK8(**it) << 6) & 0xfff;
      SGL_INCREMENT_IT_PTR(it);
      cp += (**it) & 0x3f;
      break;
    }
    SGL_INCREMENT_IT_PTR(it);
    return cp;
  };
  auto distance = [&](uint8_t* first, uint8_t* last) -> size_t {
    size_t dist;
    for (dist = 0; first < last; ++dist) {
      next(&first, nullptr);
    }
    return dist;
  };
  auto convert_8to16 = [&](uint8_t* start, uint8_t* end, uint16_t* out, size_t outsize) -> size_t {
    uint16_t* it = out;
    while (start < end) {
      uint32_t cp = next(&start, &outsize);
      if (cp > 0xffff) { /* make a surrogate pair */
        *(it++) = (uint16_t)((cp >> 10) + SGL_STRING_OP_LEAD_OFFSET);
        *(it++) = (uint16_t)((cp & 0x3ff) + SGL_STRING_OP_TRAIL_SURROGATE_MIN);
      }
      else {
        *(it++) = (uint16_t)(cp);
      }
    }

    if (outsize != 0)
      *it = 0; /* Zero termination */

    it++;
    return SGL_PTR_DIFF(it, out);
  };
  auto convert_8to32 = [&](uint8_t* start, uint8_t* end, uint32_t* out, size_t outsize) -> size_t {
    uint32_t* it = out;

    for (; start < end; ++it)
      *it = next(&start, &outsize);

    *it = 0; /* Zero termination */

    return SGL_PTR_DIFF(it, out);
  };

  size_t destLen = distance(start, end);

  /* Insufficient buffer size */
  if (destLen > out_size) {
    if (out_size != 0)
      *out = 0;
    return destLen + 1; /* zero termination */
  }

  if (sizeof(wchar_t) == 2)
    convert_8to16(start, end, (uint16_t*)out, out_size);
  else
    convert_8to32(start, end, (uint32_t*)out, out_size);

  return destLen + 1; /* zero termination */
}

/*
Code adapted from:
https://github.com/tapika/cutf/blob/master/cutf.cpp.

Converts wide string to utf-8 string.
If out == NULL, then estimate how much buffer space is needed.
Returns filled buffer length (not string length). Length includes 
extra NUL ('\0') terminating character.
*/
inline size_t 
_wchar_to_utf8(const wchar_t* s, size_t in_size, char* out, size_t out_size)
{
  const wchar_t* start = s;
  if (in_size == SIZE_MAX)
    in_size = wcslen(s);

  const wchar_t* end = start + in_size;

  auto codepoint_length = [](uint32_t cp) -> size_t {
    if (cp < 0x80) return 1;
    else if (cp < 0x800) return 2;
    else if (cp < 0x10000) return 3;
    else return 4;
  };
  auto append = [&](uint32_t cp, uint8_t* result, size_t* remain) -> uint8_t* {
    size_t charlen = codepoint_length(cp);
    /*
    If we ran out of buffer size, then we don't fill it anymore, 
    but continue iterating to get correct length
    */
    if (*remain < charlen) {
      *remain = 0;
      return result + charlen;
    }
    if (cp < 0x80) {
      /* one octet */
      *(result++) = (uint8_t)(cp);
    }
    else if (cp < 0x800) {
      /* two octets */
      *(result++) = (uint8_t)((cp >> 6) | 0xc0);
      *(result++) = (uint8_t)((cp & 0x3f) | 0x80);
    }
    else if (cp < 0x10000) {
      /* three octets */
      *(result++) = (uint8_t)((cp >> 12) | 0xe0);
      *(result++) = (uint8_t)(((cp >> 6) & 0x3f) | 0x80);
      *(result++) = (uint8_t)((cp & 0x3f) | 0x80);
    }
    else {
      /* four octets */
      *(result++) = (uint8_t)((cp >> 18) | 0xf0);
      *(result++) = (uint8_t)(((cp >> 12) & 0x3f) | 0x80);
      *(result++) = (uint8_t)(((cp >> 6) & 0x3f) | 0x80);
      *(result++) = (uint8_t)((cp & 0x3f) | 0x80);
    }
    return result;
  };

  auto convert_16to8 = [&](uint16_t* start, uint16_t* end, uint8_t* out, size_t outsize) -> size_t {
    uint8_t* it = out;
    while (start != end) {
      uint32_t cp = SGL_STRING_OP_MASK16(*start);
      ++start;
      /* Take care of surrogate pairs first */
      if (SGL_STRING_OP_IS_LEAD_SURROGATE(cp)) {
        uint32_t trail_surrogate = SGL_STRING_OP_MASK16(*start);
        ++start;
        cp = (cp << 10) + trail_surrogate + SGL_STRING_OP_SURROGATE_OFFSET;
      }
      it = append(cp, it, &outsize);
    }

    if (outsize != 0)
      *it = 0; /* Zero terminate */

    it++;
    return SGL_PTR_DIFF(it, out);
  };
  auto convert_32to8 = [&](uint32_t* start, uint32_t* end, uint8_t* out, size_t outsize) -> size_t {
    uint8_t* it = out;
    for (; start != end; ++start)
      it = append(*start, it, &outsize);

    if (outsize != 0)
      *it = 0; /* Zero terminate */

    it++;
    return SGL_PTR_DIFF(it, out);
  };

  if (sizeof(wchar_t) == 2)
    return convert_16to8((uint16_t*)start, (uint16_t*)end, (uint8_t*)out, out_size);
  else
    return convert_32to8((uint32_t*)start, (uint32_t*)end, (uint8_t*)out, out_size);
}

/**
Convert UTF-8 string (char*) to wide string (wchar_t*).
**/
inline std::wstring
utf8string_to_wstring(const std::string& in) {
  size_t out_size = _utf8_to_wchar(in.c_str(), SIZE_MAX, NULL, 0);
  wchar_t* wsz = (wchar_t*)malloc(sizeof(wchar_t) * out_size);
  _utf8_to_wchar(in.c_str(), SIZE_MAX, wsz, out_size);
  std::wstring ws = std::wstring(wsz);
  free(wsz);
  return ws;
}

/**
Convert wide string (wchar_t*) to UTF-8 string (char*).
**/
inline std::string
wstring_to_utf8string(const std::wstring& in) {
  size_t out_size = _wchar_to_utf8(in.c_str(), SIZE_MAX, NULL, 0);
  char* sz = (char*)malloc(sizeof(char) * out_size);
  _wchar_to_utf8(in.c_str(), SIZE_MAX, sz, out_size);
  std::string s = std::string(sz);
  free(sz);
  return s;
}

/**
Reads a text file (encoded in UTF-8 format) as a wide string.
**/
inline std::wstring
read_file_as_wstring(const char* file) {
  if (!file_exists(file)) {
    printf("File \"%s\" not exist.\n", file);
    return std::wstring(L"");
  }

  std::ifstream f(file);
  std::string s((std::istreambuf_iterator<char>(f)),
    std::istreambuf_iterator<char>());

  return sgl::utf8string_to_wstring(s);
}

/**
Truncate a string.
**/
inline std::string 
truncate_string(std::string str, size_t length, bool show_ellipsis = true)
{
  if (str.length() > length)
    if (show_ellipsis)
      return str.substr(0, length) + "...";
    else
      return str.substr(0, length);
  return str;
}

}; /* namespace sgl */
