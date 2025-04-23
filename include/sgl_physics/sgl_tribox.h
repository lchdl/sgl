/* triangle AABB intersection test     */

/* DO NOT include this header directly */
/* as functions defined here is not    */
/* intended to be used as API.         */

#pragma once
#include "sgl_math.h"

#define _SGL_PHYSICS_TRIBOX_INSIDE  0
#define _SGL_PHYSICS_TRIBOX_OUTSIDE 1

namespace sgl {
namespace Physics {

typedef struct { Vec3 v1; Vec3 v2; Vec3 v3; } _triangle3;
int _t_c_intersection(_triangle3 t);

};
};


