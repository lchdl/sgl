#pragma once

#include "sgl_math.h"
#include "sgl_physics/sgl_tribox.h" /* triangle AABB intersect test */

#include <cfloat>

namespace sgl {
namespace Physics {

typedef Vec3 point;
struct sphere { Vec3 p; double r; };
struct aabb { Vec3 bmin, bmax; }; /* axis aligned bounding box */
struct triangle { Vec3 p[3]; };
struct plane { Vec3 p, n; };
struct ray {
  Vec3 o, d;
  ray();
  ray(Vec3 o, Vec3 d);
};
struct ray_hit {
  bool hit;
  int entityId;
  Vec3 point, uvw, normal;
  double dist;
};

bool is_intersect(const triangle& A, const aabb& B);
bool is_intersect(const aabb& A, const aabb& B);
bool is_intersect(const ray& A, const aabb& B);
bool is_intersect(const ray& A, const triangle& B);
bool is_intersect(const ray& A, const triangle& B, double& u, double& v, double& t);
bool is_intersect(const sphere& A, const triangle& B);
bool is_intersect(const triangle& A, const plane& B);
bool is_intersect(const sphere& A, const aabb& B);

double distance_between(const sphere& A, const aabb& B);
double distance_between(const point& A, const aabb& B);
double distance_between(const point& A, const triangle& B);
double squared_distance_between(const point & A, const triangle & B);
double squared_distance_between(const point & A, const aabb & B);
double surface_area(const aabb& A);
double volume(const aabb& A);
double is_A_in_B(const point& A, const aabb& B);
bool segment_intersect_1D(const double& x1, const double& x2, const double& y1, const double& y2, double& r1, double& r2);
bool aabb_intersect(const aabb& A, const aabb& B, aabb& result);
/*
-1: all vertices smaller than value
0: triangle intersects with plane
+1: all vertices larger than value
*/
int triangle_axis_plane_test(const triangle& A, const char& axis, const double& value);

};
};

