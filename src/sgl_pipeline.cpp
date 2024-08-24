#include "sgl_pipeline.h"

namespace sgl {

Mat4x4 get_view_matrix(Vec3 eye, Vec3 look_at, Vec3 up)
{
  Vec3 front = normalize(eye - look_at);
  Vec3 left = normalize(cross(up, front));
  Vec3 up0 = normalize(cross(front, left));
  Vec3 &F = front, &L = left, &U = up0;
  const double &ex = eye.x, &ey = eye.y, &ez = eye.z;
  Mat4x4 rotation(
    L.x, L.y, L.z, 0.0,
    U.x, U.y, U.z, 0.0,
    F.x, F.y, F.z, 0.0,
    0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(
    1.0, 0.0, 0.0, -ex,
    0.0, 1.0, 0.0, -ey,
    0.0, 0.0, 1.0, -ez,
    0.0, 0.0, 0.0, 1.0);
  /*
  Important note:
  Here, we write `mul(rotation, translation)` instead of `mul(translation, rotation)`.
  The latter might seem reasonable since rotation is typically applied before translation.
  However, we apply translation first, followed by rotation, as explained in detail at:
  https://www.songho.ca/opengl/gl_camera.html.
  Note that the rotation matrix here is actually in its transposed (inverted) state,
  so please don't be confused by the order of matrix multiplication.
  */
  return mul(rotation, translation);
}
Mat4x4 get_perspective_matrix(double aspect_ratio, double near, double far, double field_of_view) {
  /* aspect_ratio = w/h */
  double inv_aspect = double(1.0) / aspect_ratio;
  double& n = near; /* near */
  double& f = far; /* far */
  double& fov = field_of_view;
  double l = -tan(fov / double(2.0)) * n; /* left */
  double r = -l; /* right */
  double t = inv_aspect * r; /* top */
  double b = -t; /* bottom */
  return Mat4x4(
    2 * n / (r - l), 0.0, (r + l) / (r - l), 0.0,
    0.0, 2 * n / (t - b), (t + b) / (t - b), 0.0,
    0.0, 0.0, -(f + n) / (f - n), -2 * f * n / (f - n),
    0.0, 0.0, -1.0, 0.0);
}

Mat4x4 get_orthographic_matrix(double near, double far, double left, double right, double top, double bottom) {
  double& n = near;
  double& f = far;
  double& r = right;
  double& l = left;
  double& t = top;
  double& b = bottom;
  return Mat4x4(
    2.0 / (r - l), 0.0, 0.0, -(r + l) / (r - l),
    0.0, 2.0 / (t - b), 0.0, -(t + b) / (t - b),
    0.0, 0.0, -2.0 / (f - n), -(f + n) / (f - n),
    0.0, 0.0, 0.0, 1.0
  );
}

Mat4x4 get_orthographic_matrix(double near, double far, double width, double height) {
  return get_orthographic_matrix(near, far, -width * 0.5, width * 0.5, height * 0.5, -height * 0.5);
}


};
