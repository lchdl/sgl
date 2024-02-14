#include "ppl_core.h"
#include <malloc.h>

namespace ppl {

Pipeline::Pipeline() {
  eye.projection_mode    = "perspective";
  surfaces.color_surface = NULL;
  surfaces.depth_surface = NULL;
}
Pipeline::~Pipeline() {}
void
Pipeline::setup_camera(const Vec3 &position, const Vec3 &look_at,
                       const Vec3 &up_dir, const double &near,
                       const double &far, const double &field_of_view) {
  eye.projection_mode  = "perspective";
  eye.projection_param = Vec3(near, far, field_of_view);
  eye.position         = position;
  eye.look_at          = look_at;
  eye.up_dir           = normalize(up_dir);
}
void
Pipeline::setup_camera(const Vec3 &position, const Vec3 &look_at,
                       const Vec3 &up_dir, const Vec3 &volume) {
  eye.projection_mode  = "orthographic";
  eye.projection_param = volume;
  eye.position         = position;
  eye.look_at          = look_at;
  eye.up_dir           = normalize(up_dir);
}

void
Pipeline::rasterize(const std::vector<Vertex> &vertex_buffer,
                    const std::vector<int32_t> &index_buffer,
                    const Mat4x4 &model_matrix, Surface &color_surface,
                    Surface &depth_surface) {

  /* Clear data from last frame. */
  ppl.Vertices.clear();
  ppl.Triangles.clear();
  ppl.Fragments.clear();

  /* Register surfaces and fill uniform variables */
  this->surfaces.color_surface = &color_surface;
  this->surfaces.depth_surface = &depth_surface;
  Uniforms uniforms;
  uniforms.model      = model_matrix;
  uniforms.view       = get_view_matrix();
  uniforms.projection = get_projection_matrix();

  /* Step 1: Vertex processing. */
  for (int i_vert = 0; i_vert < vertex_buffer.size(); i_vert++) {
    Vertex_gl vertex_out;
    /* Map vertex from model local space to homogeneous clip space and stores to
    "gl_Position". */
    vertex_shader(vertex_buffer[i_vert], uniforms, vertex_out);
    ppl.Vertices.push_back(vertex_out);
  }

  /* Step 2: Vertex post-processing. */
  for (int i_tri = 0; i_tri < ppl.Vertices.size() / 3; i_tri++) {
    /* Step 2.1: Primitive assembly. */
    Triangle_gl tri_gl;
    tri_gl.v[0] = ppl.Vertices[i_tri * 3];
    tri_gl.v[1] = ppl.Vertices[i_tri * 3 + 1];
    tri_gl.v[2] = ppl.Vertices[i_tri * 3 + 2];
    /* Step 2.2: Face culling. */
    Vec3 &p0 = tri_gl.v[0].p;
    Vec3 &p1 = tri_gl.v[1].p;
    Vec3 &p2 = tri_gl.v[2].p;
    Vec3 eyedir =
        -Vec3(uniforms.view.i31, uniforms.view.i32, uniforms.view.i33);
    if (dot(eyedir, cross(p1 - p0, p2 - p0)) > 0.0)
      continue;
    /** Step 2.3: Clipping.
    @note: For detailed explanation of how to do clipping in homogeneous space,
    see: "How to clip in homogeneous space?" in "doc/graphics_pipeline.md".
    **/
    clip_triangle(tri_gl, ppl.Triangles);
  }

  /* Step 3: Rasterization & fragment processing */
  for (int i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
    /* Step 3.1: Convert clip space to NDC space (perspective divide) */
    Triangle_gl &tri_gl = ppl.Triangles[i_tri];
    Vertex_gl &v0       = tri_gl.v[0];
    Vertex_gl &v1       = tri_gl.v[1];
    Vertex_gl &v2       = tri_gl.v[2];
    Vec3 p0_NDC         = v0.gl_Position.xyz() / v0.gl_Position.w;
    Vec3 p1_NDC         = v1.gl_Position.xyz() / v1.gl_Position.w;
    Vec3 p2_NDC         = v2.gl_Position.xyz() / v2.gl_Position.w;
    /* Step 3.2: Convert NDC space to window space */
    /**
    @note: In NDC space, x,y,z is between [-1, +1]
    NDC space       window space
    -----------------------------
    x: [-1, +1]     x: [0, +w]
    y: [-1, +1]     y: [0, +h]
    z: [-1, +1]     z: [0, +1]
    @note: The window space origin is at the lower-left corner of the screen,
    with +x axis pointing to the right and +y axis pointing to the top.
    **/
    /* clang-format off */
    const double render_width  = double(this->surfaces.color_surface->w);
    const double render_height = double(this->surfaces.color_surface->h);
    const Vec3 scale_factor    = Vec3(render_width, render_height, 1.0);
    const Vec3 iz = Vec3(1.0 / tri_gl.v[0].gl_Position.w, 
                         1.0 / tri_gl.v[1].gl_Position.w,
                         1.0 / tri_gl.v[2].gl_Position.w);
    /* clang-format on */
    const Vec4 &p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
    const Vec4 &p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
    const Vec4 &p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
    /** @note: p0, p1, p2 are actually gl_FragCoord. **/
    /* Step 3.3: Rasterization. */
    RasterRect raster_rect = get_rect(p0, p1, p2);
    Vec4 p                 = Vec4(raster_rect.x_s, raster_rect.y_s, 0.0, 0.0);
    /* precomupte: divide by real z */
    tri_gl.v[0] *= iz.i[0];
    tri_gl.v[1] *= iz.i[1];
    tri_gl.v[2] *= iz.i[2];
    for (p.y = raster_rect.y_s; p.y < raster_rect.y_e; p.y += 1.0) {
      for (p.x = raster_rect.x_s; p.x < raster_rect.x_e; p.x += 1.0) {
        /* clang-format off */
        /** @note: winding order is important.  **/
        double area = edge_function(p0, p1, p2);
        Vec3 w = Vec3(edge_function(p1, p2, p),
                      edge_function(p2, p0, p),
                      edge_function(p0, p1, p));
        /* discard pixel if it is outside the triangle area */
        if (w.i[0] < 0 || w.i[1] < 0 || w.i[2] < 0)
          continue;
        /** @note: the w_i are calculated in window space. **/
        w /= area;
        Vertex_gl v_lerp = tri_gl.v[0] * w.i[0] + tri_gl.v[1] * w.i[1] + tri_gl.v[2] * w.i[2];
        double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
        v_lerp *= z_real;
        /**
        @note: v_lerp is the interpolated vertex in homogeneous clip space
        in order to assemble the correct gl_FragCoord, we need to manually
        convert it into NDC space again.
        **/
        //print("v_lerp", v_lerp);
        /* Step 3.4: Assemble fragment and render pixel. */
        Fragment_gl fragment;
        fragment.gl_FragCoord = Vec4(p.x, 
                                     p.y, 
                                     (1.0 + v_lerp.gl_Position.z) / 2.0, 
                                     1.0 / v_lerp.gl_Position.w);
        fragment.n = v_lerp.n;
        fragment.t = v_lerp.t;
        Vec4 fragment_out;
        fragment_shader(fragment, uniforms, fragment_out);
        /* clang-format on */
        /* Step 3.5: Fragment processing */
        write_surfaces(fragment.gl_FragCoord.xy(), fragment_out,
                       fragment.gl_FragCoord.z);
      }
    }
  }
}

void
Pipeline::write_surfaces(const Vec2 &p, const Vec4 &fragment_out,
                         const double &z) {
  int w  = this->surfaces.color_surface->w;
  int h  = this->surfaces.color_surface->h;
  int ix = int(p.x);
  int iy = h - 1 - int(p.y);
  if (ix < 0 || ix >= w || iy < 0 || iy >= h)
    return;
  int pixel_id    = iy * w + ix;
  uint8_t *pixels = (uint8_t *) this->surfaces.color_surface->pixels;
  /* depth test */
  double *depths = (double *) this->surfaces.depth_surface->pixels;
  double z_orig  = depths[pixel_id];
  double z_new   = min(max(z, 0.0), 1.0);
  if (z_orig < z_new)
    return;
  depths[pixel_id] = z_new;
  uint8_t R, G, B, A;
  convert_Vec4_to_RGBA8(fragment_out, R, G, B, A);
  pixels[pixel_id * 4 + 0] = R;
  pixels[pixel_id * 4 + 1] = G;
  pixels[pixel_id * 4 + 2] = B;
  pixels[pixel_id * 4 + 3] = A;
}

void
Pipeline::clip_triangle(const Triangle_gl &triangle_in,
                        std::vector<Triangle_gl> &triangles_out) {
  std::vector<Triangle_gl> Q0, Q1;
  std::vector<Triangle_gl> *Qcur = &Q0, *Qnext = &Q1, *Qtemp = NULL;
  Qcur->push_back(triangle_in);
  Vertex_gl q[4];
  int clip_signs[2] = {+1, -1};
  for (int clip_axis = 0; clip_axis < 3; clip_axis++) {
    for (int i_clip = 0; i_clip < 2; i_clip++) {
      int clip_sign = clip_signs[i_clip];
      for (int i_tri = 0; i_tri < Qcur->size(); i_tri++) {
        Triangle_gl &tri = (*Qcur)[i_tri];
        int n_tri;
        clip_triangle(tri.v[0], tri.v[1], tri.v[2], clip_axis, clip_sign, q[0],
                      q[1], q[2], q[3], n_tri);
        if (n_tri == 1) {
          Qnext->push_back(Triangle_gl(q[0], q[1], q[2]));
        } else if (n_tri == 2) {
          Qnext->push_back(Triangle_gl(q[0], q[1], q[2]));
          Qnext->push_back(Triangle_gl(q[0], q[2], q[3]));
        }
      }
      /* swap `Qcur` and `Qnext` for the next iteration */
      Qcur->clear();
      Qtemp = Qcur;
      Qcur  = Qnext;
      Qnext = Qtemp;
    }
  }
  for (int i_tri = 0; i_tri < Qcur->size(); i_tri++)
    triangles_out.push_back((*Qcur)[i_tri]);
}

void
Pipeline::clip_triangle(const Vertex_gl &v1, const Vertex_gl &v2,
                        const Vertex_gl &v3, const int clip_axis,
                        const int clip_sign, Vertex_gl &q1, Vertex_gl &q2,
                        Vertex_gl &q3, Vertex_gl &q4, int &n_tri) {
  int p1_sign, p2_sign, p3_sign;
  if (clip_sign == +1) {
    p1_sign = (v1.gl_Position.i[clip_axis] < v1.gl_Position.w) ? +1 : -1;
    p2_sign = (v2.gl_Position.i[clip_axis] < v2.gl_Position.w) ? +1 : -1;
    p3_sign = (v3.gl_Position.i[clip_axis] < v3.gl_Position.w) ? +1 : -1;
  } else {
    p1_sign = (v1.gl_Position.i[clip_axis] > -v1.gl_Position.w) ? +1 : -1;
    p2_sign = (v2.gl_Position.i[clip_axis] > -v2.gl_Position.w) ? +1 : -1;
    p3_sign = (v3.gl_Position.i[clip_axis] > -v3.gl_Position.w) ? +1 : -1;
  }

  if (p1_sign < 0 && p2_sign < 0 && p3_sign < 0) {
    /* all triangles are clipped out */
    n_tri = 0;
  } else if (p1_sign > 0 && p2_sign > 0 && p3_sign > 0) {
    /* all triangles are contained by the upper space of the
    clipping plane, we don't need to do any clipping operations */
    n_tri = 1;
    q1 = v1, q2 = v2, q3 = v3;
  } else {
    /* clipping is needed, check how many vertices are in the upper side of
    the plane */
    n_tri = 0;
    if (p1_sign > 0)
      n_tri++;
    if (p2_sign > 0)
      n_tri++;
    if (p3_sign > 0)
      n_tri++;
    const Vertex_gl *v[3];
    /* sort pointers, ensuring that the clipping plane always intersects with
    edge v[0]-v[1] and v[0]-v[2] */
    if (n_tri == 1) {
      /* ensure v[0] is always inside the clipping plane */
      if (p1_sign > 0) {
        v[0] = &v1, v[1] = &v2, v[2] = &v3;
      } else if (p2_sign > 0) {
        v[0] = &v2, v[1] = &v3, v[2] = &v1;
      } else {
        v[0] = &v3, v[1] = &v1, v[2] = &v2;
      }
    } else {
      /* ensure v[0] is always outside the clipping plane */
      if (p1_sign < 0) {
        v[0] = &v1, v[1] = &v2, v[2] = &v3;
      } else if (p2_sign < 0) {
        v[0] = &v2, v[1] = &v3, v[2] = &v1;
      } else {
        v[0] = &v3, v[1] = &v1, v[2] = &v2;
      }
    }
    /*
    Then, clip segments p0-p1 and p0-p2.

    Assume we have two vertices A and B, segment A-B will be clipped by a
    plane, assume the intersection is C, such that C = (1-t)A + tB. Then we
    have: C_w = (1-t)*A_w + t*B_w.

    For the near plane (z-axis) clipping, we have C_z = -C_w. Since C_z =
    (1-t)*A_z + t*B_z, then we also have: -(1-t)*A_w - t*B_w = (1-t)*A_z +
    t*B_z. We can solve scalar t:

                      t = (A_z+A_w) / ((A_z+A_w)-(B_z+B_w))

    Similarily, we can solve scalar t for far plane clipping:

                      t = (A_z-A_w) / ((A_z-A_w)-(B_z-B_w))

    Clipping with other axes is also the same. Just replace A_z to A_x, A_y;
    B_z to B_x or B_y.

    Also note that this scalar t can also be used to interpolate all the
    associated vertex attributes for C. The linear interpolation is perfectly
    sufficient even in perspective distorted cases, because we are before the
    perspective divide here, were the whole perspective transformation is
    perfectly affine wrt. the 4D space we work in.
    */
    double t[2];
    clip_segment(*v[0], *v[1], clip_axis, clip_sign, t[0]);
    clip_segment(*v[0], *v[2], clip_axis, clip_sign, t[1]);

    if (n_tri == 1) {
      q1 = *(v[0]);
      q2 = Vertex_gl::lerp(*v[0], *v[1], t[0]);
      q3 = Vertex_gl::lerp(*v[0], *v[2], t[1]);
    } else { /* n_tri==2 */
      q1 = *(v[1]), q2 = *(v[2]);
      q3 = Vertex_gl::lerp(*v[0], *v[2], t[1]);
      q4 = Vertex_gl::lerp(*v[0], *v[1], t[0]);
    }
  }
}
void
Pipeline::clear_surfaces(Surface &color_surface, Surface &depth_surface,
                         const Vec4 &clear_color) {
  uint8_t R, G, B, A;
  convert_Vec4_to_RGBA8(clear_color, R, G, B, A);
  int n_pixels    = color_surface.w * color_surface.h;
  uint8_t *pixels = (uint8_t *) color_surface.pixels;
  double *depths  = (double *) depth_surface.pixels;
  for (int i = 0; i < n_pixels; i++) {
    pixels[i * 4 + 0] = R;
    pixels[i * 4 + 1] = G;
    pixels[i * 4 + 2] = B;
    pixels[i * 4 + 3] = A;
    depths[i]         = 100.0;
  }
  return;
}

Mat4x4
Pipeline::get_view_matrix() {
  Vec3 front = normalize(eye.position - eye.look_at);
  Vec3 left  = normalize(cross(eye.up_dir, front));
  Vec3 up    = normalize(cross(front, left));
  Vec3 &F = front, &L = left, &U = up;
  const double &ex = eye.position.x, &ey = eye.position.y, &ez = eye.position.z;
  Mat4x4 rotation(L.x, L.y, L.z, 0.0, U.x, U.y, U.z, 0.0, F.x, F.y, F.z, 0.0,
                  0.0, 0.0, 0.0, 1.0);
  Mat4x4 translation(1.0, 0.0, 0.0, -ex, 0.0, 1.0, 0.0, -ey, 0.0, 0.0, 1.0, -ez,
                     0.0, 0.0, 0.0, 1.0);
  Mat4x4 view_matrix = mul(rotation, translation);
  return view_matrix;
}

Mat4x4
Pipeline::get_projection_matrix() {
  Mat4x4 projection_matrix;
  if (this->eye.projection_mode == "perspective") {
    double aspect_ratio =
        double(surfaces.color_surface->w) / double(surfaces.color_surface->h);
    double inv_aspect    = double(1.0) / aspect_ratio;
    double near          = eye.projection_param.x;
    double far           = eye.projection_param.y;
    double field_of_view = eye.projection_param.z;
    double left          = -tan(field_of_view / double(2.0)) * near;
    double right         = -left;
    double top           = inv_aspect * right;
    double bottom        = -top;
    projection_matrix =
        Mat4x4(2 * near / (right - left), 0, (right + left) / (right - left), 0,
               0, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0,
               0, 0, -(far + near) / (far - near),
               -2 * far * near / (far - near), 0, 0, -1.0, 0);
    return projection_matrix;
  } else {
    /* not implemented now */
    return projection_matrix;
  }
}

};   // namespace ppl
