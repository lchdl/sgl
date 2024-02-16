#pragma once
#include <math.h>

namespace sgl {

/* Simple math operations should be inlined as much as possible. */

static const double PI = 3.1415926535897932384626;

template <typename T>
T
min(T _a, T _b) {
  return (_a < _b) ? _a : _b;
}
template <typename T>
T
max(T _a, T _b) {
  return (_a > _b) ? _a : _b;
}
template <typename T>
T
clamp(const T& _low, T& _var, const T& _high) {
  return min(max(_var, _low), _high);
}
template <typename T1, typename T2>
T1
lerp(T1 _a, T1 _b, T2 _w) {
  return (T2(1) - _w) * _a + _w * _b;
}

struct Vec2 {
  union {
    struct {
      double x, y;
    };
    struct {
      double i[2];
    };
  };
  Vec2() { x = y = 0.0; }
  Vec2(double _x, double _y) { x = _x, y = _y; }
  void operator*=(double _b) { x *= _b, y *= _b; }
};
struct Vec3 {
  union {
    struct {
      double x, y, z;
    };
    struct {
      double i[3];
    };
  };
  Vec3() { x = y = z = 0.0; }
  Vec3(double _x, double _y, double _z) { x = _x, y = _y, z = _z; }
  void operator*=(double _b) { x *= _b, y *= _b, z *= _b; }
  void operator/=(double _b) {
    double inv_b = 1.0 / _b;
    x *= inv_b, y *= inv_b, z *= inv_b;
  }
  Vec2 xy() const { return Vec2(x, y); }
  Vec2 xy() const { return Vec2(x, y); }
  Vec2 yz() const { return Vec2(y, z); }
};
struct Vec4 {
  union {
    struct {
      double x, y, z, w;
    };
    struct {
      double i[4];
    };
  };
  Vec4() { x = y = z = w = 0.0; }
  Vec4(double _x) { x = _x, y = 0.0, z = 0.0, w = 0.0; }
  Vec4(double _x, double _y) { x = _x, y = _y, z = 0.0, w = 0.0; }
  Vec4(double _x, double _y, double _z) { x = _x, y = _y, z = _z, w = 0.0; }
  Vec4(double _x, double _y, double _z, double _w) {
    x = _x, y = _y, z = _z, w = _w;
  }

  Vec4(Vec2 _a, double _b, double _c) { x = _a.x, y = _a.y, z = _b, w = _c; }
  Vec4(double _a, Vec2 _b, double _c) { x = _a, y = _b.x, z = _b.y, w = _c; }
  Vec4(double _a, double _b, Vec2 _c) { x = _a, y = _b, z = _c.x, w = _c.y; }
  Vec4(Vec3 _a, double _b) { x = _a.x, y = _a.y, z = _a.z, w = _b; }
  Vec4(double _a, Vec3 _b) { x = _a, y = _b.x, z = _b.y, w = _b.z; }
  Vec4(Vec2 _a, Vec2 _b) { x = _a.x, y = _a.y, z = _b.x, w = _b.y; }
  void operator+=(double _b) { x += _b, y += _b, z += _b, w += _b; }
  void operator-=(double _b) { x -= _b, y -= _b, z -= _b, w -= _b; }
  void operator*=(double _b) { x *= _b, y *= _b, z *= _b, w *= _b; }
  void operator/=(double _b) {
    double inv_b = 1.0 / _b;
    x *= inv_b, y *= inv_b, z *= inv_b, w *= inv_b;
  }
  Vec3 xyz() const { return Vec3(x, y, z); }
  Vec2 xy() const { return Vec2(x, y); }
  Vec2 zw() const { return Vec2(z, w); }
};
struct Mat3x3 {
  union {
    struct {
      double i[9];
    };
    struct {
      double i11, i12, i13;
      double i21, i22, i23;
      double i31, i32, i33;
    };
    struct {
      double i1x[3]; /* 1st row */
      double i2x[3]; /* 2nd row */
      double i3x[3]; /* 3rc row */
    };
  };

  Mat3x3() {
    for (int t = 0; t < 9; t++) i[t] = 0.0;
  }
  Mat3x3(double _11, double _12, double _13, double _21, double _22, double _23,
         double _31, double _32, double _33) {
    i11 = _11, i12 = _12, i13 = _13;
    i21 = _21, i22 = _22, i23 = _23;
    i31 = _31, i32 = _32, i33 = _33;
  }
};
struct Mat4x4 {
  union {
    struct {
      double i[16];
    };
    struct {
      double i11, i12, i13, i14;
      double i21, i22, i23, i24;
      double i31, i32, i33, i34;
      double i41, i42, i43, i44;
    };
    struct {
      double i1x[4]; /* 1st row */
      double i2x[4]; /* 2nd row */
      double i3x[4]; /* 3rc row */
      double i4x[4]; /* 4th row */
    };
  };
  Mat4x4() {
    for (int t = 0; t < 16; t++) i[t] = 0.0;
  }
  Mat4x4(double _11, double _12, double _13, double _14, double _21, double _22,
         double _23, double _24, double _31, double _32, double _33, double _34,
         double _41, double _42, double _43, double _44) {
    i11 = _11, i12 = _12, i13 = _13, i14 = _14;
    i21 = _21, i22 = _22, i23 = _23, i24 = _24;
    i31 = _31, i32 = _32, i33 = _33, i34 = _34;
    i41 = _41, i42 = _42, i43 = _43, i44 = _44;
  }
  static Mat4x4 identity() {
    return Mat4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
  }
};

inline Vec2
operator+(Vec2 _a, Vec2 _b) {
  return Vec2(_a.x + _b.x, _a.y + _b.y);
}
inline Vec2
operator-(Vec2 _a, Vec2 _b) {
  return Vec2(_a.x - _b.x, _a.y - _b.y);
}
inline Vec2
operator*(Vec2 _a, double _b) {
  return Vec2(_a.x * _b, _a.y * _b);
}
inline Vec2
operator*(double _a, Vec2 _b) {
  return Vec2(_b.x * _a, _b.y * _a);
}
inline Vec2
operator/(Vec2 _a, double _b) {
  double inv_b = 1.0 / _b;
  return Vec2(_a.x * inv_b, _a.y * inv_b);
}
inline Vec2
operator-(Vec2 _a) {
  return Vec2(-_a.x, -_a.y);
}
inline double
dot(Vec2 _a, Vec2 _b) {
  return _a.x * _b.x + _a.y * _b.y;
}
inline double
length(Vec2 _a) {
  return sqrt(_a.x * _a.x + _a.y * _a.y);
}
inline double
length_sq(Vec2 _a) {
  return _a.x * _a.x + _a.y * _a.y;
}
inline Vec2
normalize(Vec2 _a) {
  double invlen = double(1.0) / length(_a);
  return Vec2(_a.x * invlen, _a.y * invlen);
}
inline Vec3
operator+(Vec3 _a, Vec3 _b) {
  return Vec3(_a.x + _b.x, _a.y + _b.y, _a.z + _b.z);
}
inline Vec3
operator+(Vec3 _a, double _b) {
  return Vec3(_a.x + _b, _a.y + _b, _a.z + _b);
}
inline Vec3
operator+(double _a, Vec3 _b) {
  return Vec3(_a + _b.x, _a + _b.y, _a + _b.z);
}
inline Vec3
operator-(Vec3 _a, Vec3 _b) {
  return Vec3(_a.x - _b.x, _a.y - _b.y, _a.z - _b.z);
}
inline Vec3
operator*(Vec3 _a, double _b) {
  return Vec3(_a.x * _b, _a.y * _b, _a.z * _b);
}
inline Vec3
operator*(double _a, Vec3 _b) {
  return Vec3(_b.x * _a, _b.y * _a, _b.z * _a);
}
inline Vec3
operator*(Vec3 _a, Vec3 _b) {
  return Vec3(_a.x * _b.x, _a.y * _b.y, _a.z * _b.z);
}
inline Vec3
operator/(Vec3 _a, double _b) {
  double inv_b = 1.0 / _b;
  return Vec3(_a.x * inv_b, _a.y * inv_b, _a.z * inv_b);
}
inline Vec3
operator-(Vec3 _a) {
  return Vec3(-_a.x, -_a.y, -_a.z);
}
inline double
dot(Vec3 _a, Vec3 _b) {
  return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z;
}
inline double
length(Vec3 _a) {
  return sqrt(_a.x * _a.x + _a.y * _a.y + _a.z * _a.z);
}
inline double
length_sq(Vec3 _a) {
  return _a.x * _a.x + _a.y * _a.y + _a.z * _a.z;
}
inline Vec3
normalize(Vec3 _a) {
  double invlen = double(1.0) / length(_a);
  return Vec3(_a.x * invlen, _a.y * invlen, _a.z * invlen);
}
inline Vec3
cross(Vec3 _a, Vec3 _b) {
  return Vec3(_a.y * _b.z - _b.y * _a.z, _a.z * _b.x - _b.z * _a.x,
              _a.x * _b.y - _b.x * _a.y);
}
inline Vec4
operator+(Vec4 _a, Vec4 _b) {
  return Vec4(_a.x + _b.x, _a.y + _b.y, _a.z + _b.z, _a.w + _b.w);
}
inline Vec4
operator-(Vec4 _a, Vec4 _b) {
  return Vec4(_a.x - _b.x, _a.y - _b.y, _a.z - _b.z, _a.w - _b.w);
}
inline Vec4
operator*(Vec4 _a, double _b) {
  return Vec4(_a.x * _b, _a.y * _b, _a.z * _b, _a.w * _b);
}
inline Vec4
operator*(double _a, Vec4 _b) {
  return Vec4(_b.x * _a, _b.y * _a, _b.z * _a, _b.w * _a);
}
inline Vec4
operator/(Vec4 _a, double _b) {
  double inv_b = 1.0 / _b;
  return Vec4(_a.x * inv_b, _a.y * inv_b, _a.z * inv_b, _a.w * inv_b);
}
inline Vec4
operator-(Vec4 _a) {
  return Vec4(-_a.x, -_a.y, -_a.z, -_a.w);
}
inline double
dot(Vec4 _a, Vec4 _b) {
  return _a.x * _b.x + _a.y * _b.y + _a.z * _b.z + _a.w * _b.w;
}
inline double
length(Vec4 _a) {
  return sqrt(_a.x * _a.x + _a.y * _a.y + _a.z * _a.z + _a.w * _a.w);
}
inline double
length_sq(Vec4 _a) {
  return _a.x * _a.x + _a.y * _a.y + _a.z * _a.z + _a.w * _a.w;
}
inline Vec4
normalize(Vec4 _a) {
  double invlen = double(1.0) / length(_a);
  return Vec4(_a.x * invlen, _a.y * invlen, _a.z * invlen, _a.w * invlen);
}
inline Mat3x3
transpose(Mat3x3 _a) {
  return Mat3x3(_a.i11, _a.i21, _a.i31, _a.i12, _a.i22, _a.i32, _a.i13, _a.i23,
                _a.i33);
}
inline Mat3x3
operator+(Mat3x3 _a, Mat3x3 _b) {
  Mat3x3 c;
  for (int i = 0; i < 9; i++) c.i[i] = _a.i[i] + _b.i[i];
  return c;
}
inline Mat3x3
operator-(Mat3x3 _a, Mat3x3 _b) {
  Mat3x3 c;
  for (int i = 0; i < 9; i++) c.i[i] = _a.i[i] - _b.i[i];
  return c;
}
inline Mat3x3
mul(Mat3x3 _a, Mat3x3 _b) {
  Mat3x3 c;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      double sum = 0.0;
      for (int k = 0; k < 3; k++) sum += _a.i[i * 3 + k] * _b.i[k * 3 + j];
      c.i[i * 3 + j] = sum;
    }
  }
  return c;
}
inline Vec3
mul(Vec3 _a, Mat3x3 _b) {
  return Vec3(_a.x * _b.i[0] + _a.y * _b.i[3] + _a.z * _b.i[6],
              _a.x * _b.i[1] + _a.y * _b.i[4] + _a.z * _b.i[7],
              _a.x * _b.i[2] + _a.y * _b.i[5] + _a.z * _b.i[8]);
}
inline Vec3
mul(Mat3x3 _a, Vec3 _b) {
  return Vec3(_b.x * _a.i[0] + _b.y * _a.i[1] + _b.z * _a.i[2],
              _b.x * _a.i[3] + _b.y * _a.i[4] + _b.z * _a.i[5],
              _b.x * _a.i[6] + _b.y * _a.i[7] + _b.z * _a.i[8]);
}
inline Mat3x3
operator*(Mat3x3 _a, double _b) {
  Mat3x3 c;
  for (int i = 0; i < 9; i++) c.i[i] = _a.i[i] * _b;
  return c;
}
inline Mat3x3
operator*(double _a, Mat3x3 _b) {
  Mat3x3 c;
  for (int i = 0; i < 9; i++) c.i[i] = _a * _b.i[i];
  return c;
}

inline Mat4x4
transpose(Mat4x4 _a) {
  return Mat4x4(_a.i11, _a.i21, _a.i31, _a.i41, _a.i12, _a.i22, _a.i32, _a.i42,
                _a.i13, _a.i23, _a.i33, _a.i43, _a.i14, _a.i24, _a.i34, _a.i44);
}
inline Mat4x4
operator+(Mat4x4 _a, Mat4x4 _b) {
  Mat4x4 c;
  for (int i = 0; i < 16; i++) c.i[i] = _a.i[i] + _b.i[i];
  return c;
}
inline Mat4x4
operator-(Mat4x4 _a, Mat4x4 _b) {
  Mat4x4 c;
  for (int i = 0; i < 16; i++) c.i[i] = _a.i[i] - _b.i[i];
  return c;
}
inline Mat4x4
mul(Mat4x4 _a, Mat4x4 _b) {
  Mat4x4 c;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double sum = 0.0;
      for (int k = 0; k < 4; k++) sum += _a.i[i * 4 + k] * _b.i[k * 4 + j];
      c.i[i * 4 + j] = sum;
    }
  }
  return c;
}
inline Vec4
mul(Vec4 _a, Mat4x4 _b) {
  return Vec4(
      _a.x * _b.i[0] + _a.y * _b.i[4] + _a.z * _b.i[8] + _a.w * _b.i[12],
      _a.x * _b.i[1] + _a.y * _b.i[5] + _a.z * _b.i[9] + _a.w * _b.i[13],
      _a.x * _b.i[2] + _a.y * _b.i[6] + _a.z * _b.i[10] + _a.w * _b.i[14],
      _a.x * _b.i[3] + _a.y * _b.i[7] + _a.z * _b.i[11] + _a.w * _b.i[15]);
}
inline Vec4
mul(Mat4x4 _a, Vec4 _b) {
  return Vec4(
      _b.x * _a.i[0] + _b.y * _a.i[1] + _b.z * _a.i[2] + _b.w * _a.i[3],
      _b.x * _a.i[4] + _b.y * _a.i[5] + _b.z * _a.i[6] + _b.w * _a.i[7],
      _b.x * _a.i[8] + _b.y * _a.i[9] + _b.z * _a.i[10] + _b.w * _a.i[11],
      _b.x * _a.i[12] + _b.y * _a.i[13] + _b.z * _a.i[14] + _b.w * _a.i[15]);
}
inline Mat4x4
operator*(Mat4x4 _a, double _b) {
  Mat4x4 c;
  for (int i = 0; i < 16; i++) c.i[i] = _a.i[i] * _b;
  return c;
}
inline Mat4x4
operator*(double _a, Mat4x4 _b) {
  Mat4x4 c;
  for (int i = 0; i < 16; i++) c.i[i] = _a * _b.i[i];
  return c;
}
inline Mat4x4
operator&(Mat4x4 _a, Mat4x4 _b) {
  return mul(_a, _b);
}
inline Vec4
operator&(Mat4x4 _a, Vec4 _b) {
  return mul(_a, _b);
}
inline Vec4
operator&(Vec4 _a, Mat4x4 _b) {
  return mul(_a, _b);
}

};   // namespace sgl