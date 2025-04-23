/*

`sgl::Physcis` : implements XPBD physics engine.

The overall physics engine follows the Positional Based Dynamics (PBD),
since it is considered more stable than classical impulse based dynamics.
There are some good tutorials for this:
  https://matthias-research.github.io/pages/publications/PBDBodies.pdf
  https://www.youtube.com/watch?v=ta5RMunnbvc
Also check out the blog: 
  https://matthias-research.github.io/pages/tenMinutePhysics/index.html

Other useful links:

* Physically Based Modeling - Principles and Practice:
  classic impulse-based physics engine.
  https://graphics.stanford.edu/courses/cs448b-00-winter/papers/phys_model.pdf

* Euler method for solving ODEs (unstable):
  https://en.wikipedia.org/wiki/Euler_method

* Runge-Kutta method for solving ODEs (stable):
  https://en.wikipedia.org/wiki/Runge%E2%80%93Kutta_methods
  
*/

#pragma once

#include "sgl_physics/sgl_physics_entity.h"
#include "sgl_physics/sgl_physics_constraint.h"
#include "sgl_physics/sgl_physics_solver.h"
#include "sgl_physics/sgl_physics_debugger.h"
