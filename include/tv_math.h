#pragma once
#include "tv_types.h"
#include <math.h>

#define TV_PI tv_float(3.1415926535897932384626)

#define tv_sqrt sqrt
#define tv_tan tan

template <typename T> T min(T _a, T _b) { return (_a < _b) ? _a : _b; }
template <typename T> T max(T _a, T _b) { return (_a > _b) ? _a : _b; }

struct tv_vec2 {
  tv_float x, y;

  tv_vec2() { x = y = tv_float(0.0); }
  tv_vec2(tv_float _x, tv_float _y) { x = _x, y = _y; }
};
struct tv_vec3 {
  tv_float x, y, z;

  tv_vec3() { x = y = z = 0.0f; }
  tv_vec3(tv_float _x, tv_float _y, tv_float _z) { x = _x, y = _y, z = _z; }
};
struct tv_vec4 {
  tv_float x, y, z, w;   
  
  tv_vec4() { x = y = z = 0.0f; }
  tv_vec4(tv_float _x, tv_float _y, tv_float _z, tv_float _w) {
    x = _x, y = _y, z = _z;
    w = _w;
  }
  tv_vec4(tv_vec2 _a, tv_float _b, tv_float _c) {
    x = _a.x, y = _a.y, z = _b, w = _c;
  }
  tv_vec4(tv_float _a, tv_vec2 _b, tv_float _c) {
    x = _a, y = _b.x, z = _b.y, w = _c;
  }
  tv_vec4(tv_float _a, tv_float _b, tv_vec2 _c) {
    x = _a, y = _b, z = _c.x, w = _c.y;
  }
  tv_vec4(tv_vec3 _a, tv_float _b) {
    x = _a.x, y = _a.y, z = _a.z, w = _b;
  }
  tv_vec4(tv_float _a, tv_vec3 _b) {
    x = _a, y = _b.x, z = _b.y, w = _b.z;
  }
  tv_vec4(tv_vec2 _a, tv_vec2 _b) {
    x = _a.x, y = _a.y, z = _b.x, w = _b.y;
  }
};

struct tv_mat3x3 {
  union {
    struct { tv_float x[9]; };
    struct {
      tv_float i11, i12, i13;
      tv_float i21, i22, i23;
      tv_float i31, i32, i33;
    };
    struct {
      tv_float i1x[3]; /* 1st row */
      tv_float i2x[3]; /* 2nd row */
      tv_float i3x[3]; /* 3rc row */
    };
  };

  tv_mat3x3() {
    for (tv_u32_t i = 0; i < 9; i++)
      x[i] = tv_float(0.0);
  }
  tv_mat3x3(tv_float _11, tv_float _12, tv_float _13, 
                      tv_float _21, tv_float _22, tv_float _23, 
                      tv_float _31, tv_float _32, tv_float _33) {
    i11 = _11; i12 = _12; i13 = _13;
    i21 = _21; i22 = _22; i23 = _23;
    i31 = _31; i32 = _32; i33 = _33;
  }
};
struct tv_mat4x4 {
  union {
    struct { tv_float x[16]; };
    struct {
      tv_float i11, i12, i13, i14;
      tv_float i21, i22, i23, i24;
      tv_float i31, i32, i33, i34;
      tv_float i41, i42, i43, i44;
    };
    struct {
      tv_float i1x[4]; /* 1st row */
      tv_float i2x[4]; /* 2nd row */
      tv_float i3x[4]; /* 3rc row */
      tv_float i4x[4]; /* 4th row */
    };
  };
  
  tv_mat4x4() {
    for (tv_u32_t i = 0; i < 16; i++)
      x[i] = tv_float(0.0);
  }
  tv_mat4x4(tv_float _11, tv_float _12, tv_float _13, tv_float _14,
            tv_float _21, tv_float _22, tv_float _23, tv_float _24,
            tv_float _31, tv_float _32, tv_float _33, tv_float _34,
            tv_float _41, tv_float _42, tv_float _43, tv_float _44) {
    i11 = _11; i12 = _12; i13 = _13; i14 = _14;
    i21 = _21; i22 = _22; i23 = _23; i24 = _24;
    i31 = _31; i32 = _32; i33 = _33; i34 = _34;
    i41 = _41; i42 = _42; i43 = _43; i44 = _44;
  }
  static tv_mat4x4 identity()
  {
    return tv_mat4x4(
      1,0,0,0,
      0,1,0,0,
      0,0,1,0,
      0,0,0,1
    );
  }
};

inline tv_vec2 operator+(tv_vec2 _a, tv_vec2 _b) {
  return tv_vec2(_a.x + _b.x, _a.y + _b.y);
}
inline tv_vec2 operator-(tv_vec2 _a, tv_vec2 _b) {
  return tv_vec2(_a.x - _b.x, _a.y - _b.y);
}
inline tv_vec2 operator*(tv_vec2 _a, tv_float _b) {
  return tv_vec2(_a.x * _b, _a.y * _b);
}
inline tv_vec2 operator*(tv_float _a, tv_vec2 _b) {
  return tv_vec2(_b.x * _a, _b.y * _a);
}
inline tv_vec2 operator/(tv_vec2 _a, tv_float _b) {
  tv_float inv_b = tv_float(1.0 ) / _b;
  return tv_vec2(_a.x * inv_b, _a.y * inv_b);
}
inline tv_vec2 operator-(tv_vec2 _a) { return tv_vec2(-_a.x, -_a.y); }
inline tv_float tv_dot(tv_vec2 _a, tv_vec2 _b) {
  return _a.x * _b.x + _a.y * _b.y;
}
inline tv_float tv_length(tv_vec2 _a) {
  return tv_sqrt(_a.x * _a.x + _a.y * _a.y);
}
inline tv_float tv_length_sq(tv_vec2 _a) { return _a.x * _a.x + _a.y * _a.y; }
inline tv_vec2 tv_normalize(tv_vec2 _a) {
  tv_float invlen = tv_float(1.0) / tv_length(_a);
  return tv_vec2(_a.x * invlen, _a.y * invlen);
}

inline tv_vec3 operator+(tv_vec3 _a, tv_vec3 _b) {
  return tv_vec3(_a.x + _b.x, _a.y + _b.y, _a.z + _b.z);
}
inline tv_vec3 operator-(tv_vec3 _a, tv_vec3 _b) {
  return tv_vec3(_a.x - _b.x, _a.y - _b.y, _a.z - _b.z);
}
inline tv_vec3 operator*(tv_vec3 _a, tv_float _b) {
  return tv_vec3(_a.x * _b, _a.y * _b, _a.z * _b);
}
inline tv_vec3 operator*(tv_float _a, tv_vec3 _b) {
  return tv_vec3(_b.x * _a, _b.y * _a, _b.z * _a);
}
inline tv_vec3 operator/(tv_vec3 _a, tv_float _b) {
  return tv_vec3(_a.x / _b, _a.y / _b, _a.z / _b);
}
inline tv_vec3 operator-(tv_vec3 _a) { return tv_vec3(-_a.x, -_a.y, -_a.z); }
inline tv_float tv_dot(tv_vec3 _a, tv_vec3 _b) {
  return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z;
}
inline tv_float tv_length(tv_vec3 _a) {
  return tv_sqrt(_a.x * _a.x + _a.y * _a.y + _a.z * _a.z);
}
inline tv_float tv_length_sq(tv_vec3 _a) {
  return _a.x * _a.x + _a.y * _a.y + _a.z * _a.z;
}
inline tv_vec3 tv_normalize(tv_vec3 _a) {
  tv_float invlen = tv_float(1.0) / tv_length(_a);
  return tv_vec3(_a.x * invlen, _a.y * invlen, _a.z * invlen);
}
inline tv_vec3 tv_cross(tv_vec3 _a, tv_vec3 _b) {
  return tv_vec3(_a.y * _b.z - _b.y * _a.z, _a.z * _b.x - _b.z * _a.x,
                 _a.x * _b.y - _b.x * _a.y);
}

inline tv_vec4 operator+(tv_vec4 _a, tv_vec4 _b) {
  return tv_vec4(_a.x + _b.x, _a.y + _b.y, _a.z + _b.z, _a.w + _b.w);
}
inline tv_vec4 operator-(tv_vec4 _a, tv_vec4 _b) {
  return tv_vec4(_a.x - _b.x, _a.y - _b.y, _a.z - _b.z, _a.w - _b.w);
}
inline tv_vec4 operator*(tv_vec4 _a, tv_float _b) {
  return tv_vec4(_a.x * _b, _a.y * _b, _a.z * _b, _a.w * _b);
}
inline tv_vec4 operator*(tv_float _a, tv_vec4 _b) {
  return tv_vec4(_b.x * _a, _b.y * _a, _b.z * _a, _b.w * _a);
}
inline tv_vec4 operator/(tv_vec4 _a, tv_float _b) {
  tv_float inv_b = tv_float(1.0) / _b;
  return tv_vec4(_a.x * inv_b, _a.y * inv_b, _a.z * inv_b, _a.w * inv_b);
}
inline tv_vec4 operator-(tv_vec4 _a) {
  return tv_vec4(-_a.x, -_a.y, -_a.z, -_a.w);
}
inline tv_float tv_dot(tv_vec4 _a, tv_vec4 _b) {
  return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
}
inline tv_float tv_length(tv_vec4 _a) {
  return tv_sqrt(_a.x * _a.x + _a.y * _a.y + _a.z * _a.z + _a.w * _a.w);
}
inline tv_float tv_length_sq(tv_vec4 _a) {
  return _a.x * _a.x + _a.y * _a.y + _a.z * _a.z + _a.w * _a.w;
}
inline tv_vec4 tv_normalize(tv_vec4 _a) {
  tv_float invlen = tv_float(1.0) / tv_length(_a);
  return tv_vec4(_a.x * invlen, _a.y * invlen, _a.z * invlen, _a.w * invlen);
}

inline tv_mat3x3 tv_transpose(tv_mat3x3 _a) {
  return tv_mat3x3(_a.i11, _a.i21, _a.i31, _a.i12, _a.i22, _a.i32, _a.i13,
                   _a.i23, _a.i33);
}
inline tv_mat3x3 operator+(tv_mat3x3 _a, tv_mat3x3 _b) {
  tv_mat3x3 c;
  for (tv_u32_t i = 0; i < 9; i++)
    c.x[i] = _a.x[i] + _b.x[i];
  return c;
}
inline tv_mat3x3 operator-(tv_mat3x3 _a, tv_mat3x3 _b) {
  tv_mat3x3 c;
  for (tv_u32_t i = 0; i < 9; i++)
    c.x[i] = _a.x[i] - _b.x[i];
  return c;
}
inline tv_mat3x3 tv_mul(tv_mat3x3 _a, tv_mat3x3 _b) {
  /* this native implementation is 4x faster than the AVX version */
  tv_mat3x3 c;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      tv_float sum = tv_float(0.0);
      for (int k = 0; k < 3; k++)
        sum += _a.x[i * 3 + k] * _b.x[k * 3 + j];
      c.x[i * 3 + j] = sum;
    }
  }
  return c;
}
inline tv_vec3 tv_mul(tv_vec3 _a, tv_mat3x3 _b) {
  return tv_vec3(_a.x * _b.x[0] + _a.y * _b.x[3] + _a.z * _b.x[6],
                 _a.x * _b.x[1] + _a.y * _b.x[4] + _a.z * _b.x[7],
                 _a.x * _b.x[2] + _a.y * _b.x[5] + _a.z * _b.x[8]);
}
inline tv_vec3 tv_mul(tv_mat3x3 _a, tv_vec3 _b) {
  return tv_vec3(_b.x * _a.x[0] + _b.y * _a.x[1] + _b.z * _a.x[2],
                 _b.x * _a.x[3] + _b.y * _a.x[4] + _b.z * _a.x[5],
                 _b.x * _a.x[6] + _b.y * _a.x[7] + _b.z * _a.x[8]);
}
inline tv_mat3x3 operator*(tv_mat3x3 _a, tv_float _b) {
  tv_mat3x3 c;
  for (tv_u32_t i = 0; i < 9; i++)
    c.x[i] = _a.x[i] * _b;
  return c;
}
inline tv_mat3x3 operator*(tv_float _a, tv_mat3x3 _b) {
  tv_mat3x3 c;
  for (tv_u32_t i = 0; i < 9; i++)
    c.x[i] = _a * _b.x[i];
  return c;
}

inline tv_mat4x4 tv_transpose(tv_mat4x4 _a) {
  return tv_mat4x4(_a.i11, _a.i21, _a.i31, _a.i41, 
                   _a.i12, _a.i22, _a.i32, _a.i42, 
                   _a.i13, _a.i23, _a.i33, _a.i43, 
                   _a.i14, _a.i24, _a.i34, _a.i44);
}
inline tv_mat4x4 operator+(tv_mat4x4 _a, tv_mat4x4 _b) {
  tv_mat4x4 c;
  for (tv_u32_t i = 0; i < 16; i++)
    c.x[i] = _a.x[i] + _b.x[i];
  return c;
}
inline tv_mat4x4 operator-(tv_mat4x4 _a, tv_mat4x4 _b) {
  tv_mat4x4 c;
  for (tv_u32_t i = 0; i < 16; i++)
    c.x[i] = _a.x[i] - _b.x[i];
  return c;
}
inline tv_mat4x4 tv_mul(tv_mat4x4 _a, tv_mat4x4 _b) {
  /* this native implementation is 4x faster than the AVX version */
  tv_mat4x4 c;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      tv_float sum = tv_float(0.0);
      for (int k = 0; k < 4; k++)
        sum += _a.x[i * 4 + k] * _b.x[k * 4 + j];
      c.x[i * 4 + j] = sum;
    }
  }
  return c;
}
inline tv_vec4 tv_mul(tv_vec4 _a, tv_mat4x4 _b) {
  return tv_vec4(
      _a.x * _b.x[0] + _a.y * _b.x[4] + _a.z * _b.x[8] + _a.w * _b.x[12],
      _a.x * _b.x[1] + _a.y * _b.x[5] + _a.z * _b.x[9] + _a.w * _b.x[13],
      _a.x * _b.x[2] + _a.y * _b.x[6] + _a.z * _b.x[10] + _a.w * _b.x[14],
      _a.x * _b.x[3] + _a.y * _b.x[7] + _a.z * _b.x[11] + _a.w * _b.x[15]);
}
inline tv_vec4 tv_mul(tv_mat4x4 _a, tv_vec4 _b) {
  return tv_vec4(
      _b.x * _a.x[0] + _b.y * _a.x[1] + _b.z * _a.x[2] + _b.w * _a.x[3],
      _b.x * _a.x[4] + _b.y * _a.x[5] + _b.z * _a.x[6] + _b.w * _a.x[7],
      _b.x * _a.x[8] + _b.y * _a.x[9] + _b.z * _a.x[10] + _b.w * _a.x[11],
      _b.x * _a.x[12] + _b.y * _a.x[13] + _b.z * _a.x[14] + _b.w * _a.x[15]);
}
inline tv_mat4x4 operator*(tv_mat4x4 _a, tv_float _b) {
  tv_mat4x4 c;
  for (tv_u32_t i = 0; i < 16; i++)
    c.x[i] = _a.x[i] * _b;
  return c;
}
inline tv_mat4x4 operator*(tv_float _a, tv_mat4x4 _b) {
  tv_mat4x4 c;
  for (tv_u32_t i = 0; i < 16; i++)
    c.x[i] = _a * _b.x[i];
  return c;
}
inline tv_mat4x4 operator&(tv_mat4x4 _a, tv_mat4x4 _b) {
  return tv_mul(_a, _b);
}
inline tv_vec4 operator&(tv_mat4x4 _a, tv_vec4 _b) { return tv_mul(_a, _b); }
inline tv_vec4 operator&(tv_vec4 _a, tv_mat4x4 _b) { return tv_mul(_a, _b); }
