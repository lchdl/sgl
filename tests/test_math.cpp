#include "sgl_math.h"
#include "sgl_utils.h"
using namespace sgl;

void main() {

  Vec3 v1 = normalize(Vec3(1, 2, 3));
  Vec3 v2 = normalize(Vec3(3, 2, 1));
  Mat3x3 R = compute_rotate(v1, v2);
  print(v2);
  print(R * v1);

  Vec3 v3 = Vec3(1, 2, 3);
  Vec3 v4 = Vec3(3, 2, 4);
  print(v4);
  print(compute_transform(v3, v4).to_3x3() * v3);


  Quat q = normalize(Quat(1, 2, 3, 4));
  print(q);
  q = normalize(q);
  print(q);
  Quat q_inv = inverse(q);
  print(q_inv);
  Mat3x3 m = quat_to_mat3x3(q);
  print(m * inverse(m));
  Mat3x3 m_inv = quat_to_mat3x3(q_inv);
  print(m * m_inv);

  Quat r0 = Quat::rot_y(sgl::PI / 2);
  Quat r1 = Quat::rot_y(sgl::PI / 2 + 0.001);
  Vec3 v(0, 0, 1);
  print(rotate(rotate(v, inverse(r0)), r1));
  print(rotate(rotate(v, inverse(r0)), r0));
}

