#pragma once
#include <math.h>

namespace sgl {
/* Simple math operations should be inlined as much as possible. */

/* mathematical constants */
static const double          PI = 3.141592653589793238462643383279502884197;
static const double      PI_INV = 0.318309886183790671537767526745028724069; /* pi^-1 */
static const double PI_INV_SQRT = 0.564189583547756286948079451560772585844; /* pi^-0.5 */
static const double     PI_SQRT = 1.772453850905516027298167483341145182797; /* pi^0.5 */
static const double   PI_SQUARE = 9.869604401089358618834490999876151135313; /* pi^2 */
static const double           E = 2.718281828459045235360287471352662497757;

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
clamp(const T& _low, const T& _var, const T& _high) {
  return min(max(_var, _low), _high);
}
template <typename T1, typename T2>
T1
lerp(T1 _a, T1 _b, T2 _w) {
  return (T2(1) - _w) * _a + _w * _b;
}

inline double
radians_to_degrees(double radians) {
  return radians * 57.295779513082320876798154814105170332409;
}
inline double
degrees_to_radians(double degrees) {
  return degrees * 0.017453292519943295769236907684886127134;
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
  double& operator[](const int& _idx) { return this->i[_idx]; }
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
  void operator+=(Vec3 v) { x += v.x, y += v.y, z += v.z; }
  void operator-=(Vec3 v) { x -= v.x, y -= v.y, z -= v.z; }
  void operator*=(double _b) { x *= _b, y *= _b, z *= _b; }
  void operator/=(double _b) {
    double inv_b = 1.0 / _b;
    x *= inv_b, y *= inv_b, z *= inv_b;
  }
  Vec2 xy() const { return Vec2(x, y); }
  Vec2 yz() const { return Vec2(y, z); }
  Vec2 xz() const { return Vec2(x, z); }
  double& operator[](const int& _idx) { return this->i[_idx]; }
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
  double& operator[](const int& _idx) { return this->i[_idx]; }
};
struct IVec2 {
  union {
    struct {
      int x, y;
    };
    struct {
      int i[2];
    };
  };
  IVec2() { x = y = 0; }
  IVec2(int _x, int _y) { x = _x, y = _y; }
  void operator*=(int _b) { x *= _b, y *= _b; }
  int& operator[](const int& _idx) { return this->i[_idx]; }
};
struct IVec3 {
  union {
    struct {
      int x, y, z;
    };
    struct {
      int i[3];
    };
  };
  IVec3() { x = y = z = 0; }
  IVec3(int _x, int _y, int _z) { x = _x, y = _y, z = _z; }
  void operator*=(int _b) { x *= _b, y *= _b, z *= _b; }
  IVec2 xy() const { return IVec2(x, y); }
  IVec2 yz() const { return IVec2(y, z); }
  IVec2 xz() const { return IVec2(x, z); }
  int& operator[](const int& _idx) { return this->i[_idx]; }
};
struct IVec4 {
  union {
    struct {
      int x, y, z, w;
    };
    struct {
      int i[4];
    };
  };
  IVec4() { x = y = z = w = 0; }
  IVec4(int _x) { x = _x, y = 0, z = 0, w = 0; }
  IVec4(int _x, int _y) { x = _x, y = _y, z = 0, w = 0; }
  IVec4(int _x, int _y, int _z) { x = _x, y = _y, z = _z, w = 0; }
  IVec4(int _x, int _y, int _z, int _w) {
    x = _x, y = _y, z = _z, w = _w;
  }

  IVec4(IVec2 _a, int _b, int _c) { x = _a.x, y = _a.y, z = _b, w = _c; }
  IVec4(int _a, IVec2 _b, int _c) { x = _a, y = _b.x, z = _b.y, w = _c; }
  IVec4(int _a, int _b, IVec2 _c) { x = _a, y = _b, z = _c.x, w = _c.y; }
  IVec4(IVec3 _a, int _b) { x = _a.x, y = _a.y, z = _a.z, w = _b; }
  IVec4(int _a, IVec3 _b) { x = _a, y = _b.x, z = _b.y, w = _b.z; }
  IVec4(IVec2 _a, IVec2 _b) { x = _a.x, y = _a.y, z = _b.x, w = _b.y; }
  void operator+=(int _b) { x += _b, y += _b, z += _b, w += _b; }
  void operator-=(int _b) { x -= _b, y -= _b, z -= _b, w -= _b; }
  void operator*=(int _b) { x *= _b, y *= _b, z *= _b, w *= _b; }
  IVec3 xyz() const { return IVec3(x, y, z); }
  IVec2 xy() const { return IVec2(x, y); }
  IVec2 zw() const { return IVec2(z, w); }
  int& operator[](const int& _idx) { return this->i[_idx]; }
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
  Mat3x3(
    double _11, double _12, double _13, 
    double _21, double _22, double _23,
    double _31, double _32, double _33) {
    i11 = _11, i12 = _12, i13 = _13;
    i21 = _21, i22 = _22, i23 = _23;
    i31 = _31, i32 = _32, i33 = _33;
  }
  Mat3x3(double* _data) {
    for (int t = 0; t < 9; t++)
      this->i[t] = _data[t];
  }
  static Mat3x3 identity() {
    return Mat3x3(
        1, 0, 0, 
        0, 1, 0, 
        0, 0, 1);
  }
  inline Mat3x3 inverse() {
    double inv[9], det;
    inv[0] = (i[4] * i[8] - i[7] * i[5]);
    inv[1] = (i[2] * i[7] - i[1] * i[8]);
    inv[2] = (i[1] * i[5] - i[2] * i[4]);
    inv[3] = (i[5] * i[6] - i[3] * i[8]);
    inv[4] = (i[0] * i[8] - i[2] * i[6]);
    inv[5] = (i[3] * i[2] - i[0] * i[5]);
    inv[6] = (i[3] * i[7] - i[6] * i[4]);
    inv[7] = (i[6] * i[1] - i[0] * i[7]);
    inv[8] = (i[0] * i[4] - i[3] * i[1]);

    det = i[0] * inv[0] + i[1] * inv[3] + i[2] * inv[6];
    det = 1.0 / det;

    for (int t = 0; t < 9; t++)
      inv[t] *= det;
    return Mat3x3(inv);
  }
  inline Mat3x3 operator+=(const Mat3x3& a) {
    i11 += a.i11; i12 += a.i12; i13 += a.i13;
    i21 += a.i21; i22 += a.i22; i23 += a.i23;
    i31 += a.i31; i32 += a.i32; i33 += a.i33;
    return (*this);
  }
  inline Mat3x3 operator-=(const Mat3x3& a) {
    i11 -= a.i11; i12 -= a.i12; i13 -= a.i13;
    i21 -= a.i21; i22 -= a.i22; i23 -= a.i23;
    i31 -= a.i31; i32 -= a.i32; i33 -= a.i33;
    return (*this);
  }
  /* check if all elements in the matrix are close to zero */
  inline bool is_zero(const double eps = 1e-6) const {
    for (int t = 0; t < 9; t++) {
      if (i[t] < -fabs(eps) || i[t] > +fabs(eps))
        return false;
    }
    return true;
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
  Mat4x4(
    double _11, double _12, double _13, double _14,
    double _21, double _22, double _23, double _24,
    double _31, double _32, double _33, double _34,
    double _41, double _42, double _43, double _44) {
    i11 = _11, i12 = _12, i13 = _13, i14 = _14;
    i21 = _21, i22 = _22, i23 = _23, i24 = _24;
    i31 = _31, i32 = _32, i33 = _33, i34 = _34;
    i41 = _41, i42 = _42, i43 = _43, i44 = _44;
  }
  Mat4x4(const Mat3x3& _m) {
    i11 = _m.i11, i12 = _m.i12, i13 = _m.i13, i14 = 0.0;
    i21 = _m.i21, i22 = _m.i22, i23 = _m.i23, i24 = 0.0;
    i31 = _m.i31, i32 = _m.i32, i33 = _m.i33, i34 = 0.0;
    i41 = 0.0, i42 = 0.0, i43 = 0.0, i44 = 1.0;
  }

  Mat4x4(double* _data) {
    for (int t = 0; t < 16; t++) 
      this->i[t] = _data[t];
  }
  static Mat4x4 identity() {
    return Mat4x4(
        1, 0, 0, 0, 
        0, 1, 0, 0, 
        0, 0, 1, 0, 
        0, 0, 0, 1);
  }
  inline Mat4x4 inverse() {
    double inv[16], det;
    inv[0] = i[5] * i[10] * i[15] - i[5] * i[11] * i[14] - i[9] * i[6] * i[15] + i[9] * i[7] * i[14] + i[13] * i[6] * i[11] - i[13] * i[7] * i[10];
    inv[4] = -i[4] * i[10] * i[15] + i[4] * i[11] * i[14] + i[8] * i[6] * i[15] - i[8] * i[7] * i[14] - i[12] * i[6] * i[11] + i[12] * i[7] * i[10];
    inv[8] = i[4] * i[9] * i[15] - i[4] * i[11] * i[13] - i[8] * i[5] * i[15] + i[8] * i[7] * i[13] + i[12] * i[5] * i[11] - i[12] * i[7] * i[9];
    inv[12] = -i[4] * i[9] * i[14] + i[4] * i[10] * i[13] + i[8] * i[5] * i[14] - i[8] * i[6] * i[13] - i[12] * i[5] * i[10] + i[12] * i[6] * i[9];
    inv[1] = -i[1] * i[10] * i[15] + i[1] * i[11] * i[14] + i[9] * i[2] * i[15] - i[9] * i[3] * i[14] - i[13] * i[2] * i[11] + i[13] * i[3] * i[10];
    inv[5] = i[0] * i[10] * i[15] - i[0] * i[11] * i[14] - i[8] * i[2] * i[15] + i[8] * i[3] * i[14] + i[12] * i[2] * i[11] - i[12] * i[3] * i[10];
    inv[9] = -i[0] * i[9] * i[15] + i[0] * i[11] * i[13] + i[8] * i[1] * i[15] - i[8] * i[3] * i[13] - i[12] * i[1] * i[11] + i[12] * i[3] * i[9];
    inv[13] = i[0] * i[9] * i[14] - i[0] * i[10] * i[13] - i[8] * i[1] * i[14] + i[8] * i[2] * i[13] + i[12] * i[1] * i[10] - i[12] * i[2] * i[9];
    inv[2] = i[1] * i[6] * i[15] - i[1] * i[7] * i[14] - i[5] * i[2] * i[15] + i[5] * i[3] * i[14] + i[13] * i[2] * i[7] - i[13] * i[3] * i[6];
    inv[6] = -i[0] * i[6] * i[15] + i[0] * i[7] * i[14] + i[4] * i[2] * i[15] - i[4] * i[3] * i[14] - i[12] * i[2] * i[7] + i[12] * i[3] * i[6];
    inv[10] = i[0] * i[5] * i[15] - i[0] * i[7] * i[13] - i[4] * i[1] * i[15] + i[4] * i[3] * i[13] + i[12] * i[1] * i[7] - i[12] * i[3] * i[5];
    inv[14] = -i[0] * i[5] * i[14] + i[0] * i[6] * i[13] + i[4] * i[1] * i[14] - i[4] * i[2] * i[13] - i[12] * i[1] * i[6] + i[12] * i[2] * i[5];
    inv[3] = -i[1] * i[6] * i[11] + i[1] * i[7] * i[10] + i[5] * i[2] * i[11] - i[5] * i[3] * i[10] - i[9] * i[2] * i[7] + i[9] * i[3] * i[6];
    inv[7] = i[0] * i[6] * i[11] - i[0] * i[7] * i[10] - i[4] * i[2] * i[11] + i[4] * i[3] * i[10] + i[8] * i[2] * i[7] - i[8] * i[3] * i[6];
    inv[11] = -i[0] * i[5] * i[11] + i[0] * i[7] * i[9] + i[4] * i[1] * i[11] - i[4] * i[3] * i[9] - i[8] * i[1] * i[7] + i[8] * i[3] * i[5];
    inv[15] = i[0] * i[5] * i[10] - i[0] * i[6] * i[9] - i[4] * i[1] * i[10] + i[4] * i[2] * i[9] + i[8] * i[1] * i[6] - i[8] * i[2] * i[5];

    det = i[0] * inv[0] + i[1] * inv[4] + i[2] * inv[8] + i[3] * inv[12];
    det = 1.0 / det;

    for (int t = 0; t < 16; t++)
      inv[t] *= det;
    return Mat4x4(inv);
  }
  inline Mat4x4 operator+=(const Mat4x4& a) {
    i11 += a.i11; i12 += a.i12; i13 += a.i13; i14 += a.i14;
    i21 += a.i21; i22 += a.i22; i23 += a.i23; i24 += a.i24;
    i31 += a.i31; i32 += a.i32; i33 += a.i33; i34 += a.i34;
    i41 += a.i41; i42 += a.i42; i43 += a.i43; i44 += a.i44;
    return (*this);
  }
  inline Mat4x4 operator-=(const Mat4x4& a) {
    i11 -= a.i11; i12 -= a.i12; i13 -= a.i13; i14 -= a.i14;
    i21 -= a.i21; i22 -= a.i22; i23 -= a.i23; i24 -= a.i24;
    i31 -= a.i31; i32 -= a.i32; i33 -= a.i33; i34 -= a.i34;
    i41 -= a.i41; i42 -= a.i42; i43 -= a.i43; i44 -= a.i44;
    return (*this);
  }
  /* check if all elements in the matrix are close to zero */
  inline bool is_zero(const double eps = 1e-6) const {
    for (int t = 0; t < 16; t++) {
      if (i[t] < -fabs(eps) || i[t] > +fabs(eps))
        return false;
    }
    return true;
  }

};

/* Defines a quaternion `q` with q = s + xi + yj + zk. */
struct Quat {
  union {
    double i[4];
    struct {
      double s, x, y, z;
    };
  };

  Quat() { x = y = z = s = 0.0;}
  Quat(double _s, double _x, double _y, double _z) { this->s = _s; this->x = _x; this->y = _y; this->z = _z; }
  Quat(double _s, Vec3 _v) { this->s = _s; this->x = _v.x; this->y = _v.y; this->z = _v.z; }

  static Quat identity() { return Quat(1.0, 0.0, 0.0, 0.0); }
  static Quat rot_x(double angle) { return Quat(cos(angle / 2.0), sin(angle / 2.0), 0.0, 0.0); }
  static Quat rot_y(double angle) { return Quat(cos(angle / 2.0), 0.0, sin(angle / 2.0), 0.0); }
  static Quat rot_z(double angle) { return Quat(cos(angle / 2.0), 0.0, 0.0, sin(angle / 2.0)); }
  static Quat from_euler(double yaw, double pitch, double roll) {
    /* https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */
    /* note that euler rotations first does yaw, then pitch, finally roll (body 3-2-1 sequence). */
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);
    return Quat(
      cr * cp * cy + sr * sp * sy,
      sr * cp * cy - cr * sp * sy,
      cr * sp * cy + sr * cp * sy,
      cr * cp * sy - sr * sp * cy);
  }
  void to_euler(double& yaw, double& pitch, double& roll) {
    /* https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles */
    /* note that euler rotations first does yaw, then pitch, finally roll (body 3-2-1 sequence). */
    double sinr_cosp = 2 * (s * x + y * z);
    double cosr_cosp = 1 - 2 * (x * x + y * y);
    roll = atan2(sinr_cosp, cosr_cosp);
    double sinp = sqrt(1 + 2 * (s * y - x * z));
    double cosp = sqrt(1 - 2 * (s * y - x * z));
    pitch = 2 * atan2(sinp, cosp) - sgl::PI / 2;
    double siny_cosp = 2 * (s * z + x * y);
    double cosy_cosp = 1 - 2 * (y * y + z * z);
    yaw = atan2(siny_cosp, cosy_cosp);
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
      for (int k = 0; k < 4; k++) 
        sum += _a.i[i * 4 + k] * _b.i[k * 4 + j];
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

inline Quat
operator+(Quat q1, Quat q2) { 
  return Quat(q1.s + q2.s, q1.x + q2.x, q1.y + q2.y, q1.z + q2.z); 
}
inline Quat 
operator-(Quat q1, Quat q2) { 
  return Quat(q1.s - q2.s, q1.x - q2.x, q1.y - q2.y, q1.z - q2.z); 
}
inline Quat
operator*(Quat q, double a) {
  return Quat(q.s * a, q.x * a, q.y * a, q.z * a);
}

inline Quat operator*(double a, Quat q) {
  return Quat(a * q.s, a * q.x, a * q.y, a * q.z);
}
inline Quat
operator/(Quat q, double a) {
  return Quat(q.s / a, q.x / a, q.y / a, q.z / a);
}
inline Quat 
conjugate(Quat q) {
  return Quat(q.s, -q.x, -q.y, -q.z);
}
inline double
norm(Quat q) {
  return sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.s * q.s);
}
inline double
squared_norm(Quat q) {
  return (q.x * q.x + q.y * q.y + q.z * q.z + q.s * q.s);
}
inline Quat 
inverse(Quat q) {
  double d = squared_norm(q);
  return conjugate(q) / d;
}
inline Quat operator/(double a, Quat q) {
  return a * inverse(q);
}
inline Quat 
operator*(Quat q1, Quat q2) {
  Vec3 v1 = Vec3(q1.x, q1.y, q1.z);
  Vec3 v2 = Vec3(q2.x, q2.y, q2.z);
  double s1 = q1.s;
  double s2 = q2.s;
  return Quat(
    s1 * s2 - dot(v1, v2),
    s1 * v2 + s2 * v1 + cross(v1, v2)
  );
}
inline Quat 
operator/(Quat q1, Quat q2) {
  return q1 * inverse(q2);
}
inline Quat 
normalize(Quat q) {
  double d = norm(q);
  return q / d;
}
inline double 
dot(Quat q1, Quat q2)
{
  return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.s * q2.s;
}
inline Quat
slerp(Quat q1, Quat q2, double t) {
  /*
  Quaternion spherical interpolation (slerp) implementation adapted from Assimp.
  Also from Assimp:
    "Implementation adopted from the gmtl project. 
    All others I found on the net fail in some cases."
  */
  /* calculate cosine theta */
  double cosom = q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.s * q2.s;
  /* reverse all signs (if necessary) */
  Quat end = q2;
  if (cosom < 0.0) {
    cosom = -cosom;
    end.x = -end.x;
    end.y = -end.y;
    end.z = -end.z;
    end.s = -end.s;
  }
  /* calculate coefficients */
  double sclp, sclq;
  if ((1.0 - cosom) > 0.0001) {
    /* Standard case (slerp) */
    double omega, sinom;
    omega = acos(cosom); /* extract theta from dot product's cos theta */
    sinom = sin(omega);
    sclp = sin((1.0 - t) * omega) / sinom;
    sclq = sin(t * omega) / sinom;
  }
  else {
    /* Very close, do linear interpolation instead (because it's faster 
    and much more robust, if q1 and q2 are very close, acos will return 
    NaN sometimes, causing numerical instability). */
    sclp = 1.0 - t;
    sclq = t;
  }
  Quat q;
  q.x = sclp * q1.x + sclq * end.x;
  q.y = sclp * q1.y + sclq * end.y;
  q.z = sclp * q1.z + sclq * end.z;
  q.s = sclp * q1.s + sclq * end.s;
  return q;
}
inline Mat3x3
quat_to_mat3x3(Quat q)
{
  double t1, t2;
  Mat3x3 m;

  m.i11 = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
  m.i22 = 1.0 - 2.0 * (q.x * q.x + q.z * q.z);
  m.i33 = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);

  t1 = q.x * q.y;
  t2 = q.s * q.z;
  m.i12 = 2.0 * (t1 - t2);
  m.i21 = 2.0 * (t1 + t2);

  t1 = q.x * q.z;
  t2 = q.s * q.y;
  m.i13 = 2.0 * (t1 + t2);
  m.i31 = 2.0 * (t1 - t2);

  t1 = q.y * q.z;
  t2 = q.s * q.x;
  m.i23 = 2.0 * (t1 - t2);
  m.i32 = 2.0 * (t1 + t2);

  return m;
}
inline Quat
mat3x3_to_quat(Mat3x3 m)
{
  double tr, s;
  int di = 0;
  Quat q;

  tr = m.i11 + m.i22 + m.i33;

  if (tr >= 0) {
    s = sqrt(tr + 1.0);
    q.s = 0.5 * s;
    s = 0.5 / s;
    q.x = (m.i32 - m.i23) * s;
    q.y = (m.i13 - m.i31) * s;
    q.z = (m.i21 - m.i12) * s;
  }
  else {
    if (m.i22 > m.i11) di = 4;
    if (m.i33 > m.i[di]) di = 8;
    switch (di) {
    case 0:
      s = sqrt((m.i11 - (m.i22 + m.i33)) + 1.0);
      q.x = 0.5 * s;
      s = 0.5 / s;
      q.y = (m.i12 + m.i21) * s;
      q.z = (m.i31 + m.i13) * s;
      q.s = (m.i32 - m.i23) * s;
      break;
    case 4:
      s = sqrt((m.i22 - (m.i33 + m.i11)) + 1.0);
      q.y = 0.5 * s;
      s = 0.5 / s;
      q.z = (m.i23 + m.i32) * s;
      q.x = (m.i12 + m.i21) * s;
      q.s = (m.i13 - m.i31) * s;
      break;
    case 8:
      s = sqrt((m.i33 - (m.i11 + m.i22)) + 1.0);
      q.z = 0.5 * s;
      s = 0.5 / s;
      q.x = (m.i31 + m.i13) * s;
      q.y = (m.i23 + m.i32) * s;
      q.s = (m.i21 - m.i12) * s;
      break;
    }
  }
  return q;
}
inline Quat
mul(Vec3 v, Quat q)
{
  Quat t(0.0, v); /* expand vector to quaternion */
  return t * q;
}
/* note that euler rotations first does yaw, then pitch, finally roll (body 3-2-1 sequence). */
inline Quat
euler_to_quat(double yaw, double pitch, double roll)
{
  return Quat::from_euler(yaw, pitch, roll);
}
/* note that euler rotations first does yaw, then pitch, finally roll (body 3-2-1 sequence). */
inline void
quat_to_euler(Quat q, double& yaw, double& pitch, double& roll) {
  q.to_euler(yaw, pitch, roll);
}

}; /* namespace sgl */
