/*

sgl_convex.h : extract convex hull shape from a point cloud.
Original source code is from:

https://github.com/leomccormack/convhull_3d/blob/master/convhull_3d.h

*/

#pragma once
#include <vector>
#include "sgl_math.h"
#include "sgl_geometry.h"
#include "sgl_model.h"

#define SGL_PHYSICS_CONVHULL_MAX_NUM_FACES      100000
#define SGL_PHYSICS_CONVHULL_ND_MAX_DIMENSIONS  5
#define SGL_PHYSICS_CONVHULL_NOISE_VALUE        double(0.0000001)
#define SGL_PHYSICS_CONVHULL_VERBOSE /* enable some verbose outputs when failure */

namespace sgl {
namespace Physics {

/* definition of a 3D convex shape */
class Convex {
protected:
  std::vector<Vec3>  points; /* all vertices of the convex hull shape */
  std::vector<Vec3> normals; /* automatically generated when computing */
  std::vector<IVec3>  faces; /* face indices, each INT3 represents all vertex indices of a triangle */
public:
  bool              build_from_points(const std::vector<Vec3>& points, int precision = 4, bool verbose = false);
  bool                     export_obj(const char* file) const;
  bool                       from_obj(const char* file); /* you need to ensure to obj file's content represents a convex mesh */
  void                        destroy();
  Vec3                      get_point(int index) const;
  const std::vector<Vec3>& get_points() const;
  Vec3                     get_normal(int index) const;
  IVec3          get_triangle_indices(int index) const;
  triangle               get_triangle(const int & ind) const;
  int                      num_points() const;
  int                       num_faces() const;
  Vec3                 center_of_mass() const;
  Mat3x3               inertia_tensor(double mass, const Vec3& CoM) const;
  double              bounding_sphere(const Vec3& center) const;
public:
  Convex() {}
  virtual ~Convex() {}
};

Convex build_convex_3D(const std::vector<Vec3>& points); /* build convex shape from point cloud */
Convex build_convex_3D(const Model& model);
Convex build_convex_3D(const Mesh& mesh);
Convex build_convex_3D(const char * zip_file, const char* model_fname);

};
};

