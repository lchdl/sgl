#include "sgl_physics/sgl_gjkepa.h"

/*

References:

"Implementing GJK" by Casey Muratori:
The best description of the algorithm from the ground up
https://www.youtube.com/watch?v=Qupqu1xe7Io

"Implementing a GJK Intersection Query" by Phill Djonov
Interesting tips for implementing the algorithm
http://vec3.ca/gjk/implementation/

"GJK Algorithm 3D" by Sergiu Craitoiu
Has nice diagrams to visualise the tetrahedral case
http://in2gpu.com/2014/05/18/gjk-algorithm-3d/

"GJK + Expanding Polytope Algorithm - Implementation and Visualization"
Good breakdown of EPA with demo for visualisation
https://www.youtube.com/watch?v=6rgiPrzqt9w

*/

namespace sgl {
namespace Physics {

struct _gjk_mink {
  Vec3 sptA;  /* Result coordinate of object A's support function */
  Vec3 sptB;  /* Result coordinate of object B's support function */
  Vec3 mink;  /* Minkowski difference (sptB - sptA, a point in Minkowski space) */
};

struct _gjk_face {
  /* a triangle with all three points in Minkowski space */
  _gjk_mink v[3];  /* three vertices */
  Vec3 n;          /* normal */
};

struct _gjk_plane {
  Vec3 normal;   /* plane normal */
  double dist;     /* distance from origin */
};

_gjk_plane _gjk_build_plane(Vec3& v1, Vec3& v2, Vec3& v3) {
  _gjk_plane plane;
  Vec3 v1v2 = v2 - v1;
  Vec3 v1v3 = v3 - v1;
  plane.normal = normalize(cross(v1v2, v1v3));
  plane.dist = -dot(v1, plane.normal);
  return plane;
}

double _gjk_dist_from_plane(Vec3& point, _gjk_plane& plane) {
  /* calculate distance from point to plane */
  return dot(point, plane.normal) + plane.dist;
}

Vec3 _gjk_proj_point_to_plane(Vec3& point, _gjk_plane& plane) {
  double distance = _gjk_dist_from_plane(point, plane);
  return point - plane.normal*distance;
}

bool _gjk_realcmp(const double& a, const double& b) {
  return (fabs(a - b) < SGL_PHYSICS_GJK_REALCMP_THRESHOLD);
}

bool _gjk_vec3cmp(const Vec3& v1, const Vec3& v2) {
  if (_gjk_realcmp(v1.x, v2.x) &&
    _gjk_realcmp(v1.y, v2.y) &&
    _gjk_realcmp(v1.z, v2.z)) return true;
  else return false;
}

Vec3 _gjk_baryentric_coords(Vec3& v1, Vec3& v2, Vec3& v3, Vec3& p) {
  Vec3 coords;
  Vec3 v1v2 = v2 - v1, v1v3 = v3 - v1, v1p = p - v1;
  double d00 = dot(v1v2, v1v2);
  double d01 = dot(v1v2, v1v3);
  double d11 = dot(v1v3, v1v3);
  double d20 = dot(v1p, v1v2);
  double d21 = dot(v1p, v1v3);
  double den = d00 * d11 - d01 * d01;
  const double eps = double(1e-10);
  coords.y = (d11 * d20 - d01 * d21) / (den + eps);
  coords.z = (d00 * d21 - d01 * d20) / (den + eps);
  coords.x = 1.0 - coords.y - coords.z;
  return coords;
}

/* calculate collision point in object space */
void _gjk_solve_collision(
  /* in */
  _gjk_mink& v1, _gjk_mink& v2, _gjk_mink& v3,
  /* out */
  Vec3& localA, Vec3& localB) {

  Vec3 origin = Vec3(0, 0, 0);

  /* plane of closest triangle face */
  _gjk_plane closestPlane = _gjk_build_plane(v1.mink, v2.mink, v3.mink);

  /* projecting the origin onto the triangle (both are in Minkowski space) */
  Vec3 projectionPoint = _gjk_proj_point_to_plane(origin, closestPlane);

  /* finding the barycentric coordinate of this projection point to the triangle */
  Vec3 coords = _gjk_baryentric_coords(v1.mink, v2.mink, v3.mink, projectionPoint);
  double &u = coords.x, &v = coords.y, &w = coords.z;

  /* The contact points just have the same barycentric coordinate in their own
      triangles which are composed by result coordinates of support function */
  localA = v1.sptA * u + v2.sptA * v + v3.sptA * w;
  localB = v1.sptB * u + v2.sptB * v + v3.sptB * w;
}


/* calculate Minkowski difference from search direction */
_gjk_mink _gjk_support(Vec3& search_dir, gjk_proxy& proxyA, gjk_proxy& proxyB) {
  _gjk_mink s;
  Vec3 dir = normalize(search_dir); /* ensure the dir is normalized */
  s.sptA = proxyA.support(-dir);
  s.sptB = proxyB.support(dir);
  s.mink = s.sptB - s.sptA;
  return s;
}

/* update simplex 3 (triangle case) */
void _gjk_update_S3(_gjk_mink &a, _gjk_mink &b, _gjk_mink &c, _gjk_mink &d, int &simp_dim, Vec3 &search_dir) {
  /*
  Required winding order (counterclockwise a->b->c):
      b
      |\
      | \
      |  a
      | /
      |/
      c
  */
  Vec3 n = cross(b.mink - a.mink, c.mink - a.mink); /* triangle's normal */
  Vec3 AO = -a.mink;                                /* direction to origin */

  /* determine which feature is closest to origin, make that the new simplex */

  simp_dim = 2;
  if (dot(cross(b.mink - a.mink, n), AO) > 0) {
    /* Closest to edge AB */
    c = a;
    search_dir = cross(cross(b.mink - a.mink, AO), b.mink - a.mink);
    return;
  }
  else if (dot(cross(n, c.mink - a.mink), AO) > 0) {
    /* Closest to edge AC */
    b = a;
    search_dir = cross(cross(c.mink - a.mink, AO), c.mink - a.mink);
    return;
  }

  simp_dim = 3;
  if (dot(n, AO) > 0) {
    /* Above triangle */
    d = c;
    c = b;
    b = a;
    search_dir = n;
    return;
  }
  else {
    /* Below triangle */
    d = b;
    b = a;
    search_dir = -n;
    return;
  }
}

/* update simplex 4 (tetrahedral case) */
bool _gjk_update_S4(_gjk_mink &a, _gjk_mink &b, _gjk_mink &c, _gjk_mink &d, int &simp_dim, Vec3 &search_dir) {
  /*
  A is peak/tip of pyramid, BCD is the base (counterclockwise winding order)
  imagine the A-BCD shown below is a 3D pyramid:

                        A


                            D
                  B
                          C                 ,

  and we assume that:
  1) A is on top.
  2) B->C->D is counterclockwise if viewed from top to bottom.
  3) origin O is above BCD and below A (in the pyramid).
  */

  /* Get normals of three new faces */
  /* one of them could be the next search direction */
  Vec3 ABC = cross(b.mink - a.mink, c.mink - a.mink);
  Vec3 ACD = cross(c.mink - a.mink, d.mink - a.mink);
  Vec3 ADB = cross(d.mink - a.mink, b.mink - a.mink);

  Vec3 AO = -a.mink; /* dir to origin */
  simp_dim = 3;      /* hoisting this just cause */

  /*
  Plane-test origin with 3 faces

  Note: Kind of primitive approach used here; If origin is in front of a face, just use it as the new simplex.
  We just go through the faces sequentially and exit at the first one which satisfies dot product. Not sure this
  is optimal or if edges should be considered as possible simplices? Thinking this through in my head I feel like
  this method is good enough. Makes no difference for AABBS, should test with more complex colliders.
  */
  if (dot(ABC, AO) > 0) {
    /* In front of ABC */
    d = c;
    c = b;
    b = a;
    search_dir = ABC;
    return false;
  }
  else if (dot(ACD, AO) > 0) {
    /* In front of ACD */
    b = a;
    search_dir = ACD;
    return false;
  }
  else if (dot(ADB, AO) > 0) {
    /* In front of ADB */
    c = d;
    d = b;
    b = a;
    search_dir = ADB;
    return false;
  }
  else /* inside tetrahedron; enclosed! */
    return true;
  /*
  Note: in the case where two of the faces have similar normals,
  The origin could conceivably be closest to an edge on the tetrahedron
  Right now I don't think it'll make a difference to limit our new simplices
  to just one of the faces, maybe test it later.
  */
}

/* Expanding Polytope Algorithm based on the result of GJK */
void _epa(
  /* in */
  _gjk_mink& a, _gjk_mink& b, _gjk_mink& c, _gjk_mink& d, gjk_proxy& proxyA, gjk_proxy& proxyB,
  /* out */
  gjk_result& result) {

  /* Array of faces, each with 3 verts and a normal */
  _gjk_face faces[SGL_PHYSICS_EPA_MAX_NUM_FACES];

  /*
  
  Now, initialize with final simplex from GJK.

  IMPORTANT NOTE (by lchdl):

  In very rare cases, the tetrahedron formed by points a-b-c-d may be 
  degenerate. For example, if point a has the same position as point d, 
  the tetrahedron collapses into a triangle. In such cases, we cannot 
  compute normals for the degenerate faces because they do not span a 
  proper triangle. Instead, they only define a line due to the coinciding 
  point positions. As a result, normalizing a zero vector will produce 
  NaNs, potentially causing the entire system to "explode".

  There's little we can do to resolve this degeneracy directly, so we 
  ignore the collision and return early.

  */

  /* stores the vector before normalization */
  Vec3 q; 

  /* ABC */
  faces[0].v[0] = a;
  faces[0].v[1] = b;
  faces[0].v[2] = c;
  q = cross(b.mink - a.mink, c.mink - a.mink);
  if (q.is_zero()) return;
  faces[0].n = normalize(q);
  
  /* ACD */
  faces[1].v[0] = a;
  faces[1].v[1] = c;
  faces[1].v[2] = d;
  q = cross(c.mink - a.mink, d.mink - a.mink);
  if (q.is_zero()) return;
  faces[1].n = normalize(q);
  
  /* ADB */
  faces[2].v[0] = a;
  faces[2].v[1] = d;
  faces[2].v[2] = b;
  q = cross(d.mink - a.mink, b.mink - a.mink);
  if (q.is_zero()) return;
  faces[2].n = normalize(q);
  
  /* BDC */
  faces[3].v[0] = b;
  faces[3].v[1] = d;
  faces[3].v[2] = c;
  q = cross(d.mink - b.mink, c.mink - b.mink);
  if (q.is_zero()) return;
  faces[3].n = normalize(q);

  /* 
  now we can confirm that the case is not degenerated 
  and we can solve for the actual collision here
  */
  result.collided = true; 

  int num_faces = 4;
  int closest_face;

  for (int iterations = 0; iterations < SGL_PHYSICS_EPA_MAX_NUM_ITERATIONS; iterations++) {

    /* Find face that's closest to origin */
    double min_dist = dot(faces[0].v[0].mink, faces[0].n);
    closest_face = 0;
    for (int i = 1; i < num_faces; i++) {
      double dist = dot(faces[i].v[0].mink, faces[i].n);
      if (dist < min_dist) {
        min_dist = dist;
        closest_face = i;
      }
    }

    /* search normal to face that's closest to origin */
    Vec3 search_dir = faces[closest_face].n;
    _gjk_mink p = _gjk_support(search_dir, proxyA, proxyB);

    if (dot(p.mink, search_dir) - min_dist < SGL_PHYSICS_EPA_TOLERANCE) {
      /* converged! (new point is not significantly further from origin) */
      /* then solve collision point */
      Vec3 localA, localB;
      _gjk_solve_collision(
        faces[closest_face].v[0], faces[closest_face].v[1], faces[closest_face].v[2],
        localA, localB);
      /* calculate penetration vector */
      /* dot vertex with normal to resolve collision along normal */
      Vec3 v = faces[closest_face].n * dot(p.mink, search_dir);
      result.pA = localA;
      result.pB = localB;
      result.v = v;
      return;
    }

    /*
    if a face is deleted, there will be a "hole" on the existing
    volume, and all the edges that surround the hole is considered
    "loose edges", which are need to be fixed after removing faces
    */
    _gjk_mink loose_edges[SGL_PHYSICS_EPA_MAX_NUM_BORDER_EDGES][2];
    int num_loose_edges = 0;

    /* Find and remove all triangles that are facing p */
    for (int i = 0; i < num_faces; i++)
    {
      if (dot(faces[i].n, p.mink - faces[i].v[0].mink) > 0) {
        /*
        triangle i faces p, remove it
        Add removed triangle's edges to loose edge list.
        If it's already there, remove it (both triangles it belonged to are gone)
        */
        for (int j = 0; j < 3; j++) {
          /* Three edges per face */
          _gjk_mink current_edge[2] = { faces[i].v[j], faces[i].v[(j + 1) % 3] };
          bool found_edge = false;
          for (int k = 0; k < num_loose_edges; k++) {
            /* Check if current edge is already in list */
            if (_gjk_vec3cmp(loose_edges[k][1].mink, current_edge[0].mink) &&
              _gjk_vec3cmp(loose_edges[k][0].mink, current_edge[1].mink)) { /* reversed edge */
              /*
              Edge is already in the list, remove it
              THIS ASSUMES EDGE CAN ONLY BE SHARED BY 2 TRIANGLES (which should be true)
              THIS ALSO ASSUMES SHARED EDGE WILL BE REVERSED IN THE TRIANGLES (which
              should be true provided every triangle is wound CCW)
              */
              loose_edges[k][0] = loose_edges[num_loose_edges - 1][0]; /* Overwrite current edge */
              loose_edges[k][1] = loose_edges[num_loose_edges - 1][1]; /* with last edge in list */
              num_loose_edges--;
              found_edge = true;
              k = num_loose_edges; /* exit loop because edge can only be shared once */
            }
          }

          if (!found_edge) { /* add current edge to list */
            if (num_loose_edges >= SGL_PHYSICS_EPA_MAX_NUM_BORDER_EDGES) {
              /* we won't do any operation if edge stack is full (may be corrupted already) */
              /* otherwise the polytope won't enclose and the whole algorithm is fucked up */
              goto EPA_not_converged;
            }
            loose_edges[num_loose_edges][0] = current_edge[0];
            loose_edges[num_loose_edges][1] = current_edge[1];
            num_loose_edges++;
          }
        }

        /* Remove triangle i from list (replace current face with the last face) */
        /* and decrease num_faces counter) */
        /* the polytope should at least have 4 faces in order to form a volume */
        faces[i].v[0] = faces[num_faces - 1].v[0];
        faces[i].v[1] = faces[num_faces - 1].v[1];
        faces[i].v[2] = faces[num_faces - 1].v[2];
        faces[i].n = faces[num_faces - 1].n;
        num_faces--;
        /* decrease i so that in the next loop (i++) the newly replaced triangle is tested */
        i--;
      }
    }

    /* Reconstruct polytope with p added */
    for (int i = 0; i < num_loose_edges; i++)
    {
      if (num_faces >= SGL_PHYSICS_EPA_MAX_NUM_FACES)
        goto EPA_not_converged;

      faces[num_faces].v[0] = loose_edges[i][0];
      faces[num_faces].v[1] = loose_edges[i][1];
      faces[num_faces].v[2] = p;
      faces[num_faces].n = normalize(cross(loose_edges[i][0].mink - loose_edges[i][1].mink, loose_edges[i][0].mink - p.mink));

      /* Check for wrong normal to maintain CCW winding */
      double bias = double(0.000001); /* in case dot result is only slightly < 0 (because origin is on face) */
      if (dot(faces[num_faces].v[0].mink, faces[num_faces].n) + bias < 0) {
        _gjk_mink temp = faces[num_faces].v[0];
        faces[num_faces].v[0] = faces[num_faces].v[1];
        faces[num_faces].v[1] = temp;
        faces[num_faces].n = -faces[num_faces].n;
      }
      num_faces++;
    }
  }
EPA_not_converged:
  /* EPA didn't converge because the convex shape is too complicated */
  /* solve collision point instead and returns the currently best solution we can get */
  Vec3 localA, localB;
  _gjk_solve_collision(
    faces[closest_face].v[0], faces[closest_face].v[1], faces[closest_face].v[2],
    localA, localB);
  result.converged = false;
  result.pA = localA;
  result.pB = localB;
  result.v = faces[closest_face].n * dot(faces[closest_face].v[0].mink, faces[closest_face].n);

  return;
}

gjk_result gjk(gjk_proxy* proxyA, gjk_proxy* proxyB) {

  /* Simplex: just a set of points (a is always most recently added) */
  _gjk_mink a, b, c, d;

  /* calculate initial search direction between colliders */
  Vec3 search_dir = *(proxyA->posWorld) - *(proxyB->posWorld);
  if (search_dir.is_zero())
    search_dir = Vec3(1.0, 1.0, 1.0);

  /* initializing result assuming no collision (which is the most common case) */
  gjk_result result;
  result.converged = true;
  result.collided = false;

  /* Get initial point for simplex */
  c = _gjk_support(search_dir, *proxyA, *proxyB);
  search_dir = -c.mink; /* search in direction of origin */

  /* Get second point for a line segment simplex */
  b = _gjk_support(search_dir, *proxyA, *proxyB);

  /* we didn't reach the origin, won't enclose it */
  if (dot(b.mink, search_dir) < 0) {
    return result;
  }

  /* search perpendicular to line segment towards origin */
  search_dir = cross(cross(c.mink - b.mink, -b.mink), c.mink - b.mink);
  if (_gjk_vec3cmp(search_dir, Vec3(0, 0, 0))) {
    /* origin is on this line segment */
    /* Apparently any normal search vector will do? */
    search_dir = cross(c.mink - b.mink, Vec3(1, 0, 0)); /* normal with x-axis */
    if (_gjk_vec3cmp(search_dir, Vec3(0, 0, 0)))
      search_dir = cross(c.mink - b.mink, Vec3(0, 0, -1)); /* normal with z-axis */
  }
  int simp_dim = 2; /* simplex dimension */

  for (int iterations = 0; iterations < SGL_PHYSICS_GJK_MAX_NUM_ITERATIONS; iterations++)
  {
    a = _gjk_support(search_dir, *proxyA, *proxyB);
    if (dot(a.mink, search_dir) < 0) /* we didn't reach the origin, won't enclose it */
      return result;
    simp_dim++;
    if (simp_dim == 3) {
      _gjk_update_S3(a, b, c, d, simp_dim, search_dir);
    }
    else if (_gjk_update_S4(a, b, c, d, simp_dim, search_dir)) {
      _epa(a, b, c, d, *proxyA, *proxyB, result);
      return result;
    }
  }

  return result;
}

gjk_proxy::gjk_proxy()
{
  this->posWorld = NULL;
}

gjk_proxy::~gjk_proxy()
{
}

gjk_proxy_convex::gjk_proxy_convex()
{
  this->colLocal = NULL;
  this->q = NULL;
  this->posWorld = NULL;
}

gjk_proxy_convex::~gjk_proxy_convex()
{
}

Vec3 gjk_proxy_convex::support(const Vec3 & dir)
{
  Mat3x3 rot, rotinv;
  rot = quat_to_mat3x3(*this->q);
  rotinv = transpose(rot);
  /* Dumb O(n) support function, just brute force check all points */
  /* transform world direction vector to local space */
  Vec3 dirLocal = rotinv * dir;
  Vec3 furthest_point = colLocal->get_point(0);
  double max_dot = dot(furthest_point, dirLocal);
  for (int i = 1; i < colLocal->num_points(); i++) {
    Vec3 v = colLocal->get_point(i);
    double d = dot(v, dirLocal);
    if (d > max_dot) {
      max_dot = d;
      furthest_point = v;
    }
  }
  /* convert support to world space */
  Vec3 result = rot * furthest_point + (*(this->posWorld));
  return result;
}

gjk_proxy_sphere::gjk_proxy_sphere()
{
  this->posWorld = NULL;
  this->radius = NULL;
}

gjk_proxy_sphere::~gjk_proxy_sphere()
{
}

Vec3 gjk_proxy_sphere::support(const Vec3 & dir)
{
  return (*(this->posWorld)) + dir * (*radius);
}

gjk_proxy_box::gjk_proxy_box()
{
  size = NULL;
  mRot = mRotInv = NULL;
}

gjk_proxy_box::~gjk_proxy_box()
{
}

Vec3 gjk_proxy_box::support(const Vec3 & dir)
{
  /* transform world direction vector to local space */
  Vec3 dirLocal = (*(this->mRotInv)) * dir;
  Vec3 hsize = double(0.5) * (*(this->size));
  Vec3 local;
  local.x = ((dirLocal.x > 0.0) ? hsize.x : -hsize.x);
  local.y = ((dirLocal.y > 0.0) ? hsize.y : -hsize.y);
  local.z = ((dirLocal.z > 0.0) ? hsize.z : -hsize.z);
  /* convert support to world space */
  Vec3 result = (*(this->mRot)) * local + (*(this->posWorld));
  return result;
}

gjk_proxy_tri_static::gjk_proxy_tri_static()
{
  p[0] = p[1] = p[2] = n = NULL;
}

gjk_proxy_tri_static::~gjk_proxy_tri_static()
{
}

Vec3 gjk_proxy_tri_static::support(const Vec3 & dir)
{
  /* Find which triangle vertex is furthest along dir */
  double dot0 = dot(*p[0], dir);
  double dot1 = dot(*p[1], dir);
  double dot2 = dot(*p[2], dir);
  Vec3 P = *p[argmax3(dot0, dot1, dot2)];

  /* fake some depth behind triangle so we have volume */
  if (dot(dir, *n) < 0.0) P = P - *n;

  return P;
}

};
};
