/*
A standalone GJK-EPA algorithm implementation used for collision
detection between two convex hull shapes.
*/

#pragma once

#include "sgl_geometry.h"
#include "sgl_convex.h"

#define SGL_PHYSICS_GJK_MAX_NUM_ITERATIONS    64
#define SGL_PHYSICS_GJK_REALCMP_THRESHOLD     double(0.00001)
#define SGL_PHYSICS_EPA_TOLERANCE             double(0.0001)
#define SGL_PHYSICS_EPA_MAX_NUM_FACES         64
#define SGL_PHYSICS_EPA_MAX_NUM_BORDER_EDGES  32
#define SGL_PHYSICS_EPA_MAX_NUM_ITERATIONS    64

namespace sgl {
namespace Physics {

struct gjk_proxy
{
  /* The "gjk_proxy" structure doesn't store any data, its just some  */
  /* pointers linking to variables, so that we don't need to make a   */
  /* copy for every variable we want to reference in order to run the */
  /* GJK algorithm.                                                   */

  /* Every GJK proxy must have this to represent the world position   */
  /* the world position will be used to calculate the initial search  */
  /* direction for the GJK algorithm.                                 */
  Vec3* posWorld;  /* origin in world space (usually center of mass)  */

  gjk_proxy();
  virtual ~gjk_proxy();

  /* Support function used in GJK and EPA algorithms, must be defined */
  /* properly. "dir": (normalized) query direction.                   */
  virtual Vec3 support(const Vec3& dir) = 0;
};
struct gjk_proxy_convex : public gjk_proxy
{
  Convex* colLocal;        /* convex collision shape in object's local space */
  Quat* q;                 /* rotation (local to world) */
  gjk_proxy_convex();
  virtual ~gjk_proxy_convex();
  virtual Vec3 support(const Vec3& dir);
};
struct gjk_proxy_sphere : public gjk_proxy
{
  /* the structure itself doesn't store any data, its just some pointers linking to variables */
  double* radius;
  gjk_proxy_sphere();
  virtual ~gjk_proxy_sphere();
  virtual Vec3 support(const Vec3& dir);
};
struct gjk_proxy_box : public gjk_proxy
{
  /* axis aligned bounding box */
  Vec3* size; /* box size in x, y, z axis */
  Mat3x3* mRot, *mRotInv;  /* rotation matrix (local to world, world to local) */
  gjk_proxy_box();
  virtual ~gjk_proxy_box();
  virtual Vec3 support(const Vec3& dir);
};
struct gjk_proxy_tri_static : public gjk_proxy
{
  /* static triangle, "posWorld" is never used */

  Vec3* p[3], *n;

  gjk_proxy_tri_static();
  virtual ~gjk_proxy_tri_static();

  virtual Vec3 support(const Vec3& dir);
};

struct gjk_result
{
  /*
  collided:
  if two objects collide this will be true, and then a penetration vector
  ("v") and two collision points ("pA" and "pB") will be calculated
  */
  bool collided;
  /*
  converge:
  if EPA algorithm doesn't converge this will be false, which means that
  the penetration vector and collision points are not fully accurate
  */
  bool converged;
  /*
  v:
  penetration vector, representing how much B penetrates A, which also
  indicates the direction of the acting force (simply by normalizing v)
  contact normal can be simply obtained by normalizing the negated vector
  like this:
  >>> Vec3 n = normalize(-v); // obtain contact normal poining from A to B
  */
  Vec3 v;
  /*
  pA, pB:
  collision point on two objects A and B
  */
  Vec3 pA, pB;
};

/*
gjk: main function.
Solving collisions between two rigid bodies using GJK-EPA algorithms.
*/
gjk_result gjk(gjk_proxy* proxyA, gjk_proxy* proxyB);



};
};

