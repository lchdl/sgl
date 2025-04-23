#include "sgl_physics/sgl_geometry.h"

namespace sgl {
namespace Physics {

double distance_between(const sphere & A, const aabb & B)
{
  return distance_between(A.p, B) - A.r;
}

double distance_between(const point & A, const aabb & B)
{
  return sqrt(squared_distance_between(A, B));
}

double distance_between(const point & A, const triangle & B)
{
  return sqrt(squared_distance_between(A, B));
}

void _calcTriangleNormals(
  /* in */
  const Vec3& v1, const Vec3& v2, const Vec3& v3,
  /* out */
  Vec3& n, Vec3& n1, Vec3& n2, Vec3& n3
) {
  Vec3 v1v2 = v2 - v1;
  Vec3 v1v3 = v3 - v1;
  Vec3 v2v3 = v3 - v2;
  n = normalize(cross(v1v2, v1v3));
  n1 = normalize(cross(v1v2, n));
  n2 = normalize(cross(v2v3, n));
  n3 = normalize(cross(n, v1v3));
}

void _projectPointToPlane(
  /* in */
  const Vec3 & p, const Vec3 & o, const Vec3 & n,
  /* out */
  Vec3& j, int& s) {

  Vec3 po = o - p;
  double projlen = dot(n, po);
  Vec3 pj = n * projlen;
  j = p + pj;
  s = (projlen < 0) ? (+1) : (-1);
}

double squared_distance_between(const point & A, const triangle & B)
{
  const Vec3& v1 = B.p[0];
  const Vec3& v2 = B.p[1];
  const Vec3& v3 = B.p[2];

  Vec3 n, n1, n2, n3;
  _calcTriangleNormals(v1, v2, v3, n, n1, n2, n3);

  /* project point to triangle */
  Vec3 pj;
  int _s = 0; /* unused */
  _projectPointToPlane(A, v1, n, pj, _s);

  /* project point to edge planes */
  Vec3 j[3];
  int s[3];
  _projectPointToPlane(pj, v1, n1, j[0], s[0]);
  _projectPointToPlane(pj, v2, n2, j[1], s[1]);
  _projectPointToPlane(pj, v3, n3, j[2], s[2]);

  Vec3 q;

  if (s[0] < 0 && s[1] < 0 && s[2] < 0) {
    /* projected point j is in the triangle */
    q = pj;
  }
  else {
    int S = s[0] * s[1] * s[2];
    if (S == -1) {
      /* nearest point is one of the three vertices of the triangle */
      double d0 = DBL_MAX;
      int i0;
      for (int i = 0; i < 3; i++) {
        double d1 = length_sq(pj - B.p[i]);
        if (d0 > d1) {
          d0 = d1;
          i0 = i;
        }
      }
      q = B.p[i0];
    }
    else { /* S == +1 */
        /* nearest point is at triangle vertices or edges */
      for (int i = 0; i < 3; i++) {
        if (s[i] < 0) continue; /* this will ignore two edges */
        Vec3 vj = j[i] - B.p[i];
        Vec3 vv = B.p[(i + 1) % 3] - B.p[i];
        double a = dot(vv, vv);
        double b = dot(vj, vv);
        if (b < 0) { /* B<0, v[i] is the nearest point */
          q = B.p[i];
        }
        else if (b < a) { /* 0<B<A, j[i] is the nearest point */
          q = j[i];
        }
        else { /* B>A, v[(i + 1) % 3] is the nearest point */
          q = B.p[(i + 1) % 3];
        }
      }
    }
  }
  return length_sq(A - q);
}

double squared_distance_between(const point & A, const aabb & B)
{
  double dx = max3(B.bmin.x - A.x, 0.0, A.x - B.bmax.x);
  double dy = max3(B.bmin.y - A.y, 0.0, A.y - B.bmax.y);
  double dz = max3(B.bmin.z - A.z, 0.0, A.z - B.bmax.z);
  double dd = dx * dx + dy * dy + dz * dz;
  return dd;
}

double surface_area(const aabb & A)
{
  double dx = A.bmax.x - A.bmin.x;
  double dy = A.bmax.y - A.bmin.y;
  double dz = A.bmax.z - A.bmin.z;
  return double(2.0) * (dx * dy + dy * dz + dz * dx);
}

double volume(const aabb & A)
{
  double dx = A.bmax.x - A.bmin.x;
  double dy = A.bmax.y - A.bmin.y;
  double dz = A.bmax.z - A.bmin.z;
  return dx * dy * dz;
}

double is_A_in_B(const point & A, const aabb & B)
{
  if (B.bmin.x <= A.x && A.x <= B.bmax.x &&
    B.bmin.y <= A.y && A.y <= B.bmax.y &&
    B.bmin.z <= A.z && A.z <= B.bmax.z)
    return true;
  else return false;
}

bool segment_intersect_1D(const double & x1, const double & x2, const double & y1, const double & y2, double & r1, double & r2)
{
  /*

          y1        y2
          ------------
  -------------
  x1         x2

  */

  double A = x1 > y1 ? x1 : y1;
  double B = x2 < y2 ? x2 : y2;
  if (A > B) return false;
  else {
    r1 = A, r2 = B;
    return true;
  }
}

bool aabb_intersect(const aabb & A, const aabb & B, aabb & result)
{
  bool x = segment_intersect_1D(A.bmin.x, A.bmax.x, B.bmin.x, B.bmax.x, result.bmin.x, result.bmax.x);
  bool y = segment_intersect_1D(A.bmin.y, A.bmax.y, B.bmin.y, B.bmax.y, result.bmin.y, result.bmax.y);
  bool z = segment_intersect_1D(A.bmin.z, A.bmax.z, B.bmin.z, B.bmax.z, result.bmin.z, result.bmax.z);
  return x && y && z;
}

bool is_intersect(const triangle & A, const plane & B)
{
  Vec3 p1 = A.p[0] - B.p;
  Vec3 p2 = A.p[1] - B.p;
  Vec3 p3 = A.p[2] - B.p;
  bool sign1 = (dot(B.n, p1) > 0.0);
  bool sign2 = (dot(B.n, p2) > 0.0);
  bool sign3 = (dot(B.n, p3) > 0.0);
  if (sign1 && sign2 && sign3) return false;
  else if (!sign1 && !sign2 && !sign3) return false;
  else return true;
}

bool is_intersect(const sphere & A, const aabb & B)
{
  return distance_between(A.p, B) <= A.r ? true : false;
}

int triangle_axis_plane_test(const triangle & A, const char& axis, const double & value)
{
  int ax = axis - 'x';
  bool sign1 = (A.p[0].i[ax] > value);
  bool sign2 = (A.p[1].i[ax] > value);
  bool sign3 = (A.p[2].i[ax] > value);
  if (sign1 && sign2 && sign3) return 1;
  else if (!sign1 && !sign2 && !sign3) return -1;
  else return 0;
}

bool is_intersect(const triangle & A, const aabb & B)
{
  /* when a triangle is "on" the bounding box, the function will return false, */
  /* this is wrong so i added a small tolerance value to avoid it. */
  const double tolerance = double(0.01); /* expand box by 1% */

  Vec3 bsize = B.bmax - B.bmin;
  Vec3 bcetr = (B.bmax + B.bmin) / double(2.0);
  Vec3 zoom = 1.0 / (bsize * (1.0 + tolerance));

  triangle T;
  T.p[0] = A.p[0] - bcetr;
  T.p[1] = A.p[1] - bcetr;
  T.p[2] = A.p[2] - bcetr;

  _triangle3 t;
  t.v1 = Vec3(T.p[0].x * zoom.x, T.p[0].y * zoom.y, T.p[0].z * zoom.z);
  t.v2 = Vec3(T.p[1].x * zoom.x, T.p[1].y * zoom.y, T.p[1].z * zoom.z);
  t.v3 = Vec3(T.p[2].x * zoom.x, T.p[2].y * zoom.y, T.p[2].z * zoom.z);

  if (_t_c_intersection(t) == _SGL_PHYSICS_TRIBOX_INSIDE)
    return true;
  else
    return false;
}

bool is_intersect(const aabb & A, const aabb & B)
{
  /* x */
  double xLeft = A.bmin.x > B.bmin.x ? A.bmin.x : B.bmin.x;
  double xRight = A.bmax.x < B.bmax.x ? A.bmax.x : B.bmax.x;
  /* y */
  double yLeft = A.bmin.y > B.bmin.y ? A.bmin.y : B.bmin.y;
  double yRight = A.bmax.y < B.bmax.y ? A.bmax.y : B.bmax.y;
  /* z */
  double zLeft = A.bmin.z > B.bmin.z ? A.bmin.z : B.bmin.z;
  double zRight = A.bmax.z < B.bmax.z ? A.bmax.z : B.bmax.z;

  if (xLeft <= xRight && yLeft <= yRight && zLeft <= zRight)
    return true;
  else
    return false;
}

bool is_intersect(const ray & A, const aabb & B)
{
  double t0 = 0.0, t1 = DBL_MAX, near_t, far_t;
  Vec3 invD = 1.0 / A.d;

  near_t = (B.bmin.x - A.o.x) * invD.x;
  far_t = (B.bmax.x - A.o.x) * invD.x;
  if (near_t > far_t) swap(near_t, far_t);
  t0 = t0 < near_t ? near_t : t0;
  t1 = t1 > far_t ? far_t : t1;
  if (t0 > t1) return false;

  near_t = (B.bmin.y - A.o.y) * invD.y;
  far_t = (B.bmax.y - A.o.y) * invD.y;
  if (near_t > far_t) swap(near_t, far_t);
  t0 = t0 < near_t ? near_t : t0;
  t1 = t1 > far_t ? far_t : t1;
  if (t0 > t1) return false;

  near_t = (B.bmin.z - A.o.z) * invD.z;
  far_t = (B.bmax.z - A.o.z) * invD.z;
  if (near_t > far_t) swap(near_t, far_t);
  t0 = t0 < near_t ? near_t : t0;
  t1 = t1 > far_t ? far_t : t1;
  if (t0 > t1) return false;

  return true;
}

bool is_intersect(const ray & A, const triangle & B)
{
  double u, v, t; /* ignored */
  return is_intersect(A, B, u, v, t);
}

bool is_intersect(const ray & A, const triangle & B, double& u, double& v, double& t)
{
  const double threshold = double(1e-8);

  Vec3 e1 = B.p[1] - B.p[0];
  Vec3 e2 = B.p[2] - B.p[0];

  /* calculate plane's normal vector */
  Vec3 pvec = cross(A.d, e2);
  double det = dot(e1, pvec);
  if (fabs(det) < threshold) return false; /* ray is parallel to plane */

  double inv_det = 1.0 / det;
  Vec3 tvec = A.o - B.p[0];
  double i = dot(tvec, pvec) * inv_det;
  if (i < 0.0 || i > 1.0) return false;

  Vec3 qvec = cross(tvec, e1);
  double j = dot(A.d, qvec) * inv_det;
  if (j < 0.0 || i + j > 1.0) return false;
  t = dot(e2, qvec) * inv_det;
  if (t < 0.0) return false;
  u = 1.0 - i - j;
  v = i;

  return true;
}

bool is_intersect(const sphere & A, const triangle & B)
{
  if (A.r * A.r < squared_distance_between(A.p, B))
    return false;
  else return true;
}

ray::ray()
{
}

ray::ray(Vec3 o, Vec3 d)
{
  this->o = o;
  this->d = normalize(d);
}


};
};
