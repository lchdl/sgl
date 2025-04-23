#pragma once

#include "sgl_physics/sgl_physics_entity.h"

namespace sgl {
namespace Physics {

struct BaseConstraint
{
  virtual void solveVel(double h) = 0;
  virtual void solvePos(double h) = 0;

  BaseConstraint() {}
  virtual ~BaseConstraint() {}

};

}; /* namespace Physics */
}; /* namespace sgl */
