#pragma once

#include <vector>
#include "sgl_physics/sgl_physics_entity.h"
#include "sgl_physics/sgl_physics_constraint.h"

namespace sgl {
namespace Physics {

/*

XPBDSolver: Solving physics systems using the XPBD (Extended Position-
Based Dynamics) algorithm proposed by Matthias M¨¹ller et al.

Paper:
https://matthias-research.github.io/pages/publications/PBDBodies.pdf

Code adapted from:
https://github.com/markeasting/THREE-XPBD, with essential fixes to 
enhance friction behavior. The original implementation exhibited overly 
smooth interactions, causing bodies to slip off inclined surfaces. This 
version improves realism by allowing objects to stick to ramps under 
appropriate friction.

*/

class XPBDSolver
{

  /* 
  For simplicity, the solver is implemented as static member functions. 
  It performs calculations only and maintains no internal state.
  */

protected:
  
  /* * * * * * * * * * * * * * * * * * * * * */
  /* Broad/Narrow phase collision detection  */
  /* * * * * * * * * * * * * * * * * * * * * */

  static std::vector<Collision> _detectCollision_BroadPhase(
    std::vector<RigidBody*>& bodies, 
    double dt)
  {
    std::vector<Collision> bpcps; /* broad-phase collisions */
    int n_bodies = len(bodies);

    for (int i = 0; i < n_bodies; i++) {
      RigidBody* A = bodies[i];
      for (int j = i + 1; j < n_bodies; j++) {
        RigidBody* B = bodies[j];
        if (!B->canCollide) 
          continue;

        if (A->cid == B->cid)
          continue;

        if ((!A->isDynamic || A->isSleeping) && (!B->isDynamic || B->isSleeping))
          continue;

        Collision cp(A, B, Vec3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, 0.0), Vec3(0.0, 0.0, 0.0));
        
        /* maximum relative velocity */
        const double max_rvel = 
          max(length(A->vel), length(A->prevVel)) + 
          max(length(B->vel), length(B->prevVel));

        const double safe_distance = max(dt * max_rvel, 0.001);
        
        double d1 = length(A->pose.p - B->pose.p);
        double d2 = A->collider.radius + B->collider.radius;
        if (d1 < d2 + safe_distance) {
          bpcps.push_back(cp);
        }
      }
    }

    return bpcps;
  }

  static std::vector<Collision> _detectCollision_NarrowPhase(
    std::vector<Collision>& cps)
  {
    std::vector<Collision> npcps; /* narrow-phase collisions */
    int n_cps = len(cps);
    for (int i = 0; i < n_cps; i++)
    {
      ColliderType colTypeA = cps[i].A->collider.colliderType;
      ColliderType colTypeB = cps[i].B->collider.colliderType;

      if (colTypeA == ColliderType_ConvexMesh && colTypeB == ColliderType_ConvexMesh) {
        /*
        Case 1: Solve collisions between two rigid bodies.
        */
        if (_solve_collision_RigidBody_vs_RigidBody(cps[i].A, cps[i].B, cps[i]))
          npcps.push_back(cps[i]);
      }
      else if (colTypeA == ColliderType_Sphere && colTypeB == ColliderType_ConvexMesh) {
        /*
        Case 2: Sphere vs. RigidBody.

        Since sphere collision can also be solved using GJK, we also use 
        RigidBody collision detection algorithm.

        Note: Collision detection between spheres and rigid bodies is not 
        very accurate. You may notice that during simulation, these objects 
        sometimes bounce off each other at high speeds, even when they are 
        initially at rest.
        */
        if (_solve_collision_RigidBody_vs_RigidBody(cps[i].A, cps[i].B, cps[i]))
          npcps.push_back(cps[i]);
      }
      else if (colTypeA == ColliderType_ConvexMesh && colTypeB == ColliderType_Sphere) {
        /*
        Case 3: RigidBody vs. Sphere.
        The same with Case 2.
        */
        if (_solve_collision_RigidBody_vs_RigidBody(cps[i].A, cps[i].B, cps[i]))
          npcps.push_back(cps[i]);
      }
      else if (colTypeA == ColliderType_Sphere && colTypeB == ColliderType_Sphere) {
        /*
        Case 4: Sphere vs. Sphere.
        */
        if (_solve_collision_Sphere_vs_Sphere(cps[i].A, cps[i].B, cps[i]))
          npcps.push_back(cps[i]);
      }
      /* TODO: add other collisions. */
      else {
        /* Maybe I should print a warning here? */
      }
    }
    return npcps;
  }
  
  /* * * * * * * * * * */
  /* XPBD core solver  */
  /* * * * * * * * * * */

  /*
  _applyBodyPairCorrection: 
  If `onlyCalculateDeltaLambda = true`, this function only 
  computes and returns the delta lambda. No correction will 
  be applied in this case.
  */
  static double _applyBodyPairCorrection(
    RigidBody* body0,
    RigidBody* body1,
    Vec3 corr,
    double compliance,
    double dt,
    Vec3* pos0 = NULL,
    Vec3* pos1 = NULL,
    bool velocityLevel = false,
    bool onlyCalculateDeltaLambda = false)
  {
    double C = length(corr);
    if (C < 0.000001)
      return 0.0;

    Vec3 n = normalize(corr);

    double w0 = body0 ? body0->getGeneralizedInverseMass(n, pos0) : 0.0;
    double w1 = body1 ? body1->getGeneralizedInverseMass(n, pos1) : 0.0;

    double w = w0 + w1;
    if (w == 0.0)
      return 0.0;

    /*
    (3.3.1) Lagrange multiplier.
    Equation (4) was simplified because a single constraint 
    iteration is used (initial lambda = 0).
    */
    double dlambda = -C / (w + compliance / dt / dt);

    if (!onlyCalculateDeltaLambda) {
      n *= -dlambda;

      if (body0) body0->applyCorrection(n, pos0, velocityLevel);
      if (body1) body1->applyCorrection(-n, pos1, velocityLevel);
    }

    return dlambda;
  }

  static void _solvePenetrationAndFriction(
    Collision& contact,
    double h) 
  {
    /* * * * * * * * * * * * * * * * * * * * */
    /* Solve Penetration (Normal Direction)  */
    /* * * * * * * * * * * * * * * * * * * * */

    /* (26) - p1, p2 and penetration depth (d) are calculated here. */
    contact.update();

    /* (3.5) if d <= 0 we skip the contact */
    if (contact.d <= 0.0)
      return;

    /* (3.5) Resolve penetration (¦¤x = dn using a = 0 and ¦Ën) */
    Vec3 dx = -contact.n * contact.d;
    
    double delta_lambda_n = _applyBodyPairCorrection(
      contact.A,
      contact.B,
      dx,
      0.0,
      h,
      &contact.p1,
      &contact.p2,
      false
    );

    /* (5) Update Lagrange multiplier */
    contact.lambda_n += delta_lambda_n;

    /* * * * * * * * * * * * * * * * * * * * */
    /* Solve Friction (Tangential Direction) */
    /* * * * * * * * * * * * * * * * * * * * */

    contact.update();

    double delta_lambda_t = _applyBodyPairCorrection(
      contact.A,
      contact.B,
      dx,
      0.0,
      h,
      &contact.p1,
      &contact.p2,
      false,
      true
    );
    contact.lambda_t += delta_lambda_t;

    /*
    "...but only if ¦Ë_t < ¦Ì_s * ¦Ë_n."
    Note:
      This inequation was flipped because the lambda values are always negative!
      With 1 position iteration (XPBD), lambda_t is always zero!
     */
    if (contact.lambda_t > contact.staticFriction * contact.lambda_n) {

      /* (26) Positions in current state and before the substep integration */
      Vec3 p1prev = contact.A->prevPose.p + rotate(contact.r1, contact.A->prevPose.q);
      Vec3 p2prev = contact.B->prevPose.p + rotate(contact.r2, contact.B->prevPose.q);
      
      /* (27) Relative motion */
      Vec3 dp = (contact.p1 - p1prev) - (contact.p2 - p2prev);
      
      /* (28) Tangential component of relative motion */
      Vec3 dp_t = dp - contact.n * dot(dp, contact.n);
      
      /* Note: Had to negate dp_t to get correct results */
      dp_t = -dp_t;

      _applyBodyPairCorrection(
        contact.A,
        contact.B,
        dp_t,
        0.0,
        h,
        &contact.p1,
        &contact.p2,
        false
      );
    }

  }
  
  static void _solvePositions(
    std::vector<Collision>& contacts, 
    double h)
  {
    int n_contacts = len(contacts);
    for (int i = 0; i < n_contacts; i++)
      _solvePenetrationAndFriction(contacts[i], h);
  }

  static void _solveVelocities(
    std::vector<Collision>& contacts,
    double h,
    Vec3 gravity)
  {
    int n_contacts = len(contacts);
    for (int i = 0; i < n_contacts; i++) {
      Collision& contact = contacts[i];

      contact.update();

      Vec3 dv = Vec3(0.0, 0.0, 0.0);

      /* 
      (29) Relative normal and tangential velocities.
      Note: v and vn are recalculated since the velocities were
      modified by RigidBody.update() in the meantime.
      */
      Vec3 v = contact.A->getVelocityAt(contact.p1) - contact.B->getVelocityAt(contact.p2);
      double vn = dot(v, contact.n);
      Vec3 vt = v - contact.n * vn;
      double vt_len = length(vt);

      /*
      (30) Friction
      */
      if (vt_len > 0.000001) {
        double Fn = -contact.lambda_n / (h * h);
        double friction = min(h * contact.dynamicFriction * Fn, vt_len);
        dv -= normalize(vt) * friction;
      }

      /* 
      (34) Restitution
      To avoid jittering we set e = 0 if vn is small (`threshold`).
      */
      double threshold = 2.0 * length(gravity) * h;
      double e = (fabs(contact.vn) <= threshold) ? 0.0 : contact.e;
      double vn_tilde = contact.vn;
      double restitution = -vn + min(-e * vn_tilde, 0.0);
      dv += contact.n * restitution;

      /* (33) Velocity update */
      _applyBodyPairCorrection(
        contact.A,
        contact.B,
        dv,
        0.0,
        h,
        &contact.p1,
        &contact.p2,
        true
      );
    }
  }

public:

  /* * * * * * * * * * */
  /* XPBD entry point  */
  /* * * * * * * * * * */

  static void update(
    std::vector<RigidBody*>& bodies,
    std::vector<BaseConstraint*>& constraints,
    double dt, int substeps,
    Vec3 gravity)
  {
   
    /* TODO: add positional and rotation damping. */

    double h = dt / substeps;

    /* 
    (3.5) To save computational cost we collect potential collision 
    pairs once per time step instead of once per sub-step using a 
    tree of axis aligned bounding boxes.
    */
    std::vector<Collision> collisions = _detectCollision_BroadPhase(bodies, dt);
    
    /*
    XPBD main loop
    */
    int n_bodies = len(bodies);
    int n_constraints = len(constraints);

    for (int i = 0; i < substeps; i++) {

      for (int j = 0; j < n_bodies; j++)
        bodies[j]->integrate(h, gravity);

      /*
      
      Important Note: narrow-phase collision detection should always 
      be performed AFTER the body->integrate() process. To understand 
      why, consider the following scenario:
      
      * A highly elastic "superball" falls toward the floor. 
      
      If collision detection is performed before integration, the 
      following sequence occurs:
      
      1. Collision detected: The system registers a collision because 
         the ball intersects the floor.
      2. Integration step: The ball's position updates, causing it to 
         sink further into the floor due to its downward velocity.
      3. _solveVelocities() adjusts the ball's velocity, flipping it 
         upward.

      Which causes the problem:
      Since the ball is still penetrating the floor, the next collision 
      check immediately detects another collision (duplicate detection).
      This creates a feedback loop: each frame, the ball collides 
      repeatedly, appearing "stuck" to the ground despite its high 
      bounciness. The simulation becomes inaccurate, as the ball fails
      to rebound cleanly.

      So the solution is to perform narrow-phase collision detection 
      AFTER body->integrate() and every thing will be fine.
      
      */

      std::vector<Collision> contacts = _detectCollision_NarrowPhase(collisions);

      for (int j = 0; j < n_constraints; j++)
        constraints[j]->solvePos(h);

      _solvePositions(contacts, h);

      for (int j = 0; j < n_bodies; j++)
        bodies[j]->update(h);

      for (int j = 0; j < n_constraints; j++)
        constraints[j]->solveVel(h);

      _solveVelocities(contacts, h, gravity);
    }

  }

};

}; /* namespace Physics */
}; /* namespace sgl */
