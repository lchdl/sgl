#pragma once

/*

sgl_physics_entity.h: Defines entity objects used in our physics simulation system.

This header includes:
  1. Various entity types
  2. Collision detection algorithms for these entities

Since entities are always involved in collision handling, the collision detection
algorithms are implemented directly in this header.

Entity Types:

  RigidBody:
    Represents an object with linear and angular velocity. It can move and rotate.
    The collider for this type is a convex hull.

  FixedMesh:
    A special type of RigidBody that is static (non-movable).
    It is typically used as the world mesh to represent large scenery in games.
    There are no constraints on its shape, it can be enclosed or open and made up
    of arbitrary triangle sets. We do not assume the mesh is convex, as game
    environments can be complex.

    Because of this complexity, collisions between RigidBody and FixedMesh cannot
    be resolved using GJK-EPA. Some games use approximation techniques instead.
    For example, Re-Volt uses a set of ~30 spheres to loosely enclose a RigidBody
    and tests collisions between these spheres and the FixedMesh (i.e., sphere-
    triangle collisions). The positions and radii of these spheres are manually 
    defined by developers through trial and error.

    In SGL, we do not adopt this approach due to its lack of automation and heavy
    reliance on manual labor. Instead, we directly use the convex vertices of the
    RigidBody for collision testing with the FixedMesh. However, this method can
    introduce inaccuracies. For example, if a convex shape collides with a spike
    on the mesh, it may result in significant error due to the lack of edge 
    collision detection for performance reasons.

*/

#include "sgl_math.h"
#include "sgl_model.h"
#include "sgl_physics/sgl_gjkepa.h"

namespace sgl {
namespace Physics {

struct Pose
{
  Vec3 p; /* position */
  Quat q; /* rotation */
  
  Pose() {};
  Pose(Vec3 p, Quat q) { this->p = p; this->q = q; }
  virtual ~Pose() {}
  /* translate a vector inplace */
  void translate(Vec3& v) { v += p; }
  /* inverse translate a vector inplace */
  void invTranslate(Vec3& v) { v -= p; }
  /* rotate a vector inplace */
  void rotate(Vec3& v) { v = sgl::rotate(v, q); }
  /* inverse rotate a vector inplace */
  void invRotate(Vec3& v) { v = sgl::rotate(v, inverse(q)); }
  /* transform a vector inplace using current pose position and rotation */
  void transform(Vec3& v) { v = sgl::rotate(v, q); v += p; }
  /* inverse transform a vector inplace using current pose position and rotation */
  void invTransform(Vec3& v) { v -= p; v = sgl::rotate(v, inverse(q)); }
  /* stack two poses to form a new pose */
  void transformPose(Pose& pose) { pose.q = q * pose.q; rotate(pose.p); pose.p += p; };
};

enum ColliderType {
  ColliderType_Undefined,
  ColliderType_Sphere,
  ColliderType_ConvexMesh,
};

struct Collider
{
  double radius;             /* broad phase  */
  ColliderType colliderType; /* narrow phase */
  struct {
    /* ColliderType_ConvexMesh */
    struct { gjk_proxy_convex gjkProxy; Convex convexHull; } convexMeshCollider;
    /* ColliderType_Sphere */
    struct { gjk_proxy_sphere gjkProxy; } sphereCollider;
  };

  Collider() {
    colliderType = ColliderType_Undefined;
  }
  virtual ~Collider() {}
};

class InertiaTensorSolver {

public:

  /* https://en.wikipedia.org/wiki/List_of_moments_of_inertia */

  static Mat3x3 cone(const double& r, const double& h, const double& mass) {
    /* radius, height, mass, pointing upward */
    /* https://www.physicsforums.com/threads/calculating-the-inertia-tensor-of-cone-with-uniform-density.749602/ */
    double A = h / r; /* Cot(alpha) */
    double A2 = A * A;
    double B = (2.0 * A2 + 3.0) / (10.0 * A);
    return mass * r * r * Mat3x3::diag(B, B, 0.6);
  }

  static Mat3x3 box(const double& x, const double& y, const double& z, const double& mass) {
    double XY = x * x + y * y;
    double XZ = x * x + z * z;
    double YZ = y * y + z * z;
    return mass / 12.0 * Mat3x3::diag(YZ, XZ, XY);
  }

  static Mat3x3 cube(const double& l, const double& mass) {
    return InertiaTensorSolver::box(l, l, l, mass);
  }

  static Mat3x3 sphere(const double& r, const double& mass) {
    return 0.4 * mass * r * r * Mat3x3::identity();
  }

  static Mat3x3 convex(const Convex& convex, const double& mass, const Vec3& center_of_mass) {
    return convex.inertia_tensor(mass, center_of_mass);
  }
};

struct RigidBody 
{
  /* basic properties */
  int cid;                /* collision group id (objects with the same id will not collide with each other) */
  Pose pose;              /* position and rotation */
  Vec3 vel;               /* linear velocity (m/s) */
  Vec3 omega;             /* angular velocity (rad/s) */
  double invMass;         /* inverse mass (kg^-1) */
  Mat3x3 invLocalInertia; /* inverse local inertia tensor */
  Vec3 force;             /* total external force (N) */
  Vec3 torque;            /* total external torque (N*m) */
  double gravity;         /* coefficient applied to g = 9.80665 m/s^2, defaults to 1.0 */
  double staticFriction;  /* static friction coefficient (0.0~1.0) */
  double dynamicFriction; /* dynamic friction coefficient (0.0~1.0) */
  double restitution;     /* restitution coefficient (0.0~1.0) */
  Collider collider;      /* rigid body collider */
  /* prev states */
  Pose prevPose;          /* pose in previous state */
  Vec3 prevVel;           /* linear velocity in previous state */
  Vec3 prevOmega;         /* angular velocity in previous state */
  /* other properties */
  bool canCollide;        /* if this object can collide with other objects */
  bool canSleep;          /* if this object can enter sleep mode */
  bool isDynamic;         /* `isDynamic = false` means this object is fixed */
  bool isSleeping;        /* if this object currently sleeping */
  bool hasStableContact;  /* if this object has stable contact with other objects */
  /* visualization & debugging */
  /*
  `sgl::Model` pointer, will be used in rendering.
  NOTE: model's center of mass should be placed at the origin.
  */
  sgl::Model* model;      /* Not owned, can be NULL. */
  Vec3 modelOffset;       /* Rigid body's actual center of mass in the object's local space.
                             Used to correct the model when the center of mass is not at the
                             local origin. This offset is automatically applied during
                             rendering, but must be computed manually by the user. */
  double scale;           /* scaling factor, model should be scaled by this factor when rendering.  */
  std::string name;

  /* member functions */
  RigidBody() { reset(); }
  virtual ~RigidBody() {}
  /* reset rigid body states */
  void reset() {
    cid = -1;
    pose.p = Vec3(0.0, 0.0, 0.0);
    pose.q = Quat::identity();
    prevPose = pose;
    vel = Vec3(0.0, 0.0, 0.0);
    prevVel = vel;
    omega = Vec3(0.0, 0.0, 0.0);
    prevOmega = omega;
    invMass = 1.0;
    invLocalInertia = Mat3x3::identity();
    force = Vec3(0.0, 0.0, 0.0);
    torque = Vec3(0.0, 0.0, 0.0);
    gravity = 1.0;
    staticFriction = 1.00;
    dynamicFriction = 0.99;
    restitution = 0.9;
    collider.colliderType = ColliderType_Undefined;
    collider.convexMeshCollider.gjkProxy.colLocal = &collider.convexMeshCollider.convexHull;
    collider.convexMeshCollider.gjkProxy.posWorld = &pose.p;
    collider.convexMeshCollider.gjkProxy.q = &pose.q;
    collider.sphereCollider.gjkProxy.posWorld = &pose.p;
    collider.sphereCollider.gjkProxy.radius = &collider.radius;
    isDynamic = true;
    canCollide = true;
    hasStableContact = false;
    canSleep = true;
    isSleeping = false;
    model = NULL;
    modelOffset = Vec3(0.0, 0.0, 0.0);
    scale = 1.0;
    name = "<unnamed>";
  }
  gjk_proxy* getGJKCollider() {
    if (collider.colliderType == ColliderType_ConvexMesh)
      return &collider.convexMeshCollider.gjkProxy;
    else if (collider.colliderType == ColliderType_Sphere)
      return &collider.sphereCollider.gjkProxy;
    else
      return NULL;
  }
  void buildConvex(
    int id,
    const Convex& convex_hull, 
    double mass, 
    Vec3 CoM_position, 
    double scale = 1.0,
    sgl::Model* model = NULL,
    const std::string& name = "<unnamed>") 
  {
    reset();
    this->cid = id;
    std::vector<Vec3> convex_points = convex_hull.get_points();
    for (int i = 0; i < (int)convex_points.size(); i++)
      convex_points[i] = scale * (convex_points[i] - CoM_position);
    this->collider.colliderType = ColliderType_ConvexMesh;
    this->collider.convexMeshCollider.convexHull = sgl::Physics::build_convex_3D(convex_points);
    this->collider.radius = collider.convexMeshCollider.convexHull.bounding_sphere(Vec3(0.0, 0.0, 0.0));
    this->model = model;
    this->modelOffset = CoM_position;
    this->scale = scale;
    /* now convex hull is placed at the origin */
    this->invMass = 1.0 / mass;
    Mat3x3 localInertia = this->collider.convexMeshCollider.convexHull.inertia_tensor(mass, Vec3(0.0, 0.0, 0.0));
    this->invLocalInertia = inverse(localInertia);
    this->setName(name);
  }
  void buildSphere(
    int id,
    double radius,
    double mass,
    double scale = 1.0,
    sgl::Model* model = NULL,
    const Vec3& model_offset = Vec3(0.0, 0.0, 0.0),
    const std::string& name = "<unnamed>"
  ) {
    reset();
    this->cid = id;
    this->collider.colliderType = ColliderType_Sphere;
    this->collider.radius = radius * scale;
    this->model = model;
    this->modelOffset = model_offset;
    this->scale = scale;
    /* now convex hull is placed at the origin */
    this->invMass = 1.0 / mass;
    Mat3x3 localInertia = InertiaTensorSolver::sphere(this->collider.radius, mass);
    this->invLocalInertia = inverse(localInertia);
    this->setName(name);
  }
  double mass() const { return 1.0 / invMass; }
  Mat3x3 invInertia() const {
    /* 
    Equation expressed in LaTeX: 
    $$ \mathbf{R} \mathbf{I}^{-1} \mathbf{R}^{-1} $$
    */
    return 
      quat_to_mat3x3(this->pose.q) * 
      this->invLocalInertia * 
      quat_to_mat3x3(inverse(this->pose.q));
  };
  Mat3x3 inertia() const {
    /*
    Equation expressed in LaTeX:
    $$ \mathbf{R}^{-1} \mathbf{I} \mathbf{R} $$
    */
    return 
      quat_to_mat3x3(inverse(this->pose.q)) *
      inverse(this->invLocalInertia) *
      quat_to_mat3x3(this->pose.q);
  }
  void setPosition(Vec3 posWorld) {
    pose.p = posWorld;
    prevPose = pose;
  }
  void setRotation(Quat qWorld) {
    pose.q = normalize(qWorld);
    prevPose = pose;
  }
  void setName(const std::string& name) {
    this->name = name;
  }
  void setOmega(Vec3 omega) {
    this->omega = omega;
  }
  void setVelocity(Vec3 vel) {
    this->vel = vel;
  }
  void setStatic() {
    isDynamic = false;
    gravity = 0.0;
    invMass = 0.0;
    invLocalInertia = Mat3x3(
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0,
      0.0, 0.0, 0.0
    );
  }
  Vec3 getVelocityAt(Vec3 posWorld) const {
    if (!isDynamic) 
      return Vec3(0.0, 0.0, 0.0);
    return vel + cross(omega, posWorld - pose.p);
  }
  /*
  calculate generalized inverse mass in world space
  `normal` and `pos` (can be NULL) should all in world space
  */
  double getGeneralizedInverseMass(Vec3 normal, Vec3* pos = NULL) const {
    if (!isDynamic)
      return 0.0;
    Vec3 rxn = Vec3(0.0, 0.0, 0.0);
    if (pos == NULL) {
      rxn = normal;
    }
    else {
      rxn = cross(*pos - pose.p, normal);
    }
    return invMass + dot(rxn, invInertia() * rxn);
  }
  /*
  apply correction to body.
  if `velocityLevel = false`, apply position correction, otherwise velocity correction.
  */
  void applyCorrection(Vec3 corr, Vec3* pos = NULL, bool velocityLevel = false) {
    if (!isDynamic)
      return;
    Vec3 dq = Vec3(0.0, 0.0, 0.0);
    if (pos == NULL) {
      dq = corr;
    }
    else {
      if (velocityLevel) {
        vel += corr * invMass;
      }
      else {
        pose.p += corr * invMass;
      }
      dq = cross(*pos - pose.p, corr);
    }

    dq = invInertia() * dq;

    if (velocityLevel) {
      omega += dq;
    }
    else {
      applyRotation(dq);
    }
  }
  void applyRotation(Vec3 rot, double scale = 1.0) {
    /*
    Safety clamping. This happens very rarely if the solver
    wants to turn the body by more than 30 degrees in the
    orders of milliseconds.
    */
    double maxPhi = 0.5;
    double phi = length(rot);
    if (phi * scale > maxPhi) {
      scale = maxPhi / phi;
    }
    Quat dq = Quat(
      0.0, 
      rot.x * scale, 
      rot.y * scale, 
      rot.z * scale
    );
    dq = dq * pose.q;
    pose.q = Quat(
      pose.q.s + 0.5 * dq.s,
      pose.q.x + 0.5 * dq.x,
      pose.q.y + 0.5 * dq.y,
      pose.q.z + 0.5 * dq.z
    );
    pose.q = normalize(pose.q);
  }
  /* Euler integration of unconstrained object motion */
  void integrate(double dt, Vec3 gravity) {
    if (!isDynamic)
      return;
    prevPose = pose;
    if (isSleeping)
      return;
    /* Euler step */
    vel += gravity * this->gravity * dt;
    vel += force * invMass * dt;
    pose.p += vel * dt;
    omega += invInertia() * torque * dt;
    applyRotation(omega, dt);
  }
  void update(double dt) {
    if (!isDynamic || isSleeping)
      return;

    /* Store the current velocities (required for the velocity solver) */
    prevVel = vel;
    prevOmega = omega;

    /* Calculate velocity based on position change */
    vel = (pose.p - prevPose.p) / dt;

    Quat dq = pose.q * inverse(prevPose.q);
    omega = Vec3(dq.x * 2.0 / dt, dq.y * 2.0 / dt, dq.z * 2.0 / dt);
    if (dq.s < 0.0) {
      omega = -omega;
    }
  }
  void sleep() {
    if (isSleeping)
      return;
    vel = Vec3(0.0, 0.0, 0.0);
    omega = Vec3(0.0, 0.0, 0.0);
    prevVel = Vec3(0.0, 0.0, 0.0);
    prevOmega = Vec3(0.0, 0.0, 0.0);
    isSleeping = true;
  }
  void wake() {
    if (!isSleeping)
      return;
    isSleeping = false;
  }
  void applyForceW(Vec3 force, Vec3 worldPos) {
    wake();
    this->force += force;
    this->torque += cross(worldPos - pose.p, force);
  }
  Vec3 localToWorld(Vec3 v) {
    return pose.p + rotate(v, pose.q);
  }
  Vec3 worldToLocal(Vec3 v) {
    return rotate(v - pose.p, inverse(pose.q));
  }
};

struct Collision
{
  RigidBody* A;
  RigidBody* B;
  double lambda;   /* lambda */
  double lambda_n; /* lambda N (normal) */
  double lambda_t; /* lambda T (tangential) */
  Vec3 r1;         /* Contact point (local, on A) */
  Vec3 r2;         /* Contact point (local, on B) */
  Vec3 p1;         /* Contact point (world, on A) */
  Vec3 p2;         /* Contact point (world, on B) */
  Vec3 n;          /* Contact normal */
  double d;        /* penetration depth */
  Vec3 vrel;       /* Relative velocity */
  double vn;       /* Normal velocity */
  double e;        /* Coefficient of restitution */
  double staticFriction;
  double dynamicFriction;
  Vec3 F;          /* Current constraint force */
  double Fn;       /* Current constraint force (normal direction) == -contact.lambda_n / (h * h) */

  /*
  Recalculate collision information to ensure 
  `p_A` and `p_B` are up-to-date.
  */
  void update()
  {
    Vec3 p1 = A->pose.p + rotate(r1, A->pose.q);
    Vec3 p2 = B->pose.p + rotate(r2, B->pose.q);
    this->p1 = p1;
    this->p2 = p2;
    /* (3.5) Penetration depth, should be positive, if not, reverse normal direction. */
    this->d = dot(p1 - p2, n);
  }

  Collision(RigidBody* A, RigidBody* B,
    Vec3 normal, Vec3 p1, Vec3 p2, Vec3* r1 = NULL, Vec3* r2 = NULL)
  {
    assert(A && B);
    assert(A != B);
    assert(A->cid != B->cid);

    this->lambda = 0.0;
    this->lambda_n = 0.0;
    this->lambda_t = 0.0;
    this->d = 0.0;
    this->Fn = 0.0;

    this->A = A;
    this->B = B;
    this->p1 = p1;
    this->p2 = p2;
    this->r1 = r1 ? (*r1) : (A->worldToLocal(p1));
    this->r2 = r2 ? (*r2) : (B->worldToLocal(p2));
    this->n = normal;
    this->vrel = A->getVelocityAt(p1) - B->getVelocityAt(p2);
    this->vn = dot(vrel, n);
    this->e = (A->restitution + B->restitution) / 2.0;
    this->staticFriction = (A->staticFriction + B->staticFriction) / 2.0;
    this->dynamicFriction = (A->dynamicFriction + B->dynamicFriction) / 2.0;

    this->update();
  }
  virtual ~Collision() {}
};

/*
If two bodies `A` and `B` collides, returns true and fill in `cp`.
*/
inline bool _solve_collision_RigidBody_vs_RigidBody(
  RigidBody* A, RigidBody* B, Collision& cp) 
{
  gjk_result r = sgl::Physics::gjk(
    A->getGJKCollider(), 
    B->getGJKCollider()
  );
  if (r.collided) {
    cp = Collision(A, B, normalize(r.pA - r.pB), r.pA, r.pB);
  }
  return r.collided;
}

inline bool _solve_collision_Sphere_vs_Sphere(
  RigidBody* A, RigidBody* B, Collision& cp)
{
  /* 
  Sphere-sphere collisions can be efficiently solved analytically, 
  so GJK is unnecessary due to its slower speed and lower accuracy.
  */
  Vec3 d = B->pose.p - A->pose.p;
  double d_AB = length(d);
  if (d_AB == 0.0)
    d = Vec3(1.0, 1.0, 1.0);
  d = normalize(d);
  Vec3 pA = A->pose.p + d * A->collider.radius;
  Vec3 pB = B->pose.p - d * B->collider.radius;
  /*
  Note: While d points from A to B, the direction from pA to pB
  is B to A, since in penetration, pA lies on A but inside B,
  and pB lies on B but inside A.
  */
  if (d_AB < A->collider.radius + B->collider.radius) {
    cp = Collision(A, B, d, pA, pB);
    return true;
  }
  else {
    return false;
  }
}

}; /* namespace Physics */
}; /* namespace sgl */
