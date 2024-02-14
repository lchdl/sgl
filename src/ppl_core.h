/*
  == The life of a triangle ==
  A complete pipeline to draw a single triangle
  on screen. This is also the rasterization pipeline
  of standard OpenGL.
*/
#pragma once
#include "ppl_math.h"
#include "ppl_shader.h"
#include "ppl_texture.h"
#include "ppl_utils.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace ppl {

class Pipeline {
public:
  /**
  Setup a perspective camera. Must be called before `rasterize(...)`.
  @param position: Eye position.
  @param look_at: Eye view target.
  @param up_dir: Eye up vector (normalized).
  @param near, far: Near / far clipping plane.
  @param field_of_view: Field of view (in degrees).
  **/
  void setup_camera(const Vec3 &position, const Vec3 &look_at,
                    const Vec3 &up_dir, const double &near, const double &far,
                    const double &field_of_view);
  /**
  Setup a orthographic camera. Must be called before `rasterize(...)`.
  @param position: Eye position.
  @param look_at: Eye view target.
  @param up_dir: Eye up vector (normalized).
  @param volume: Viewing volume (dx, dy, dz).
  **/
  void setup_camera(const Vec3 &position, const Vec3 &look_at,
                    const Vec3 &up_dir, const Vec3 &volume);
  /**
  Clear surfaces.
  @param color_surface: Color surface to be cleared.
  @param depth_surface: Depth surface to be cleared.
  @param clear_color: Color that will be filled to the color surface.
  **/
  void clear_surfaces(Surface &color_surface, Surface &depth_surface,
                      const Vec4 &clear_color);
  /**
  Rasterize a single triangle.
  @param vertex_buffer, index_buffer: The buffers describe the model.
  @param model_matrix: The model transformation applied before rendering.
  @param color_surface: The color buffer, surface format should be RGBA8.
  @param depth_surface: The depth buffer, surface format should be float64.
  @note: the size of the color and depth buffer should be the same.
  **/
  void rasterize(const std::vector<Vertex> &vertex_buffer,
                 const std::vector<int32_t> &index_buffer,
                 const Mat4x4 &model_matrix, Surface &color_surface,
                 Surface &depth_surface);

protected:
  /**
  Calculate view and projection matrices.
  **/
  Mat4x4 get_view_matrix();
  Mat4x4 get_projection_matrix();
  /**
  Clip triangle in homogeneous space.
  @note: Assume each vertex has homogeneous coordinate (x,y,z,w), then clip
  points outside -w <= x, y, z <= +w.
  @param triangle_in: Input triangle in homogeneous space.
  @param triangles_out: Output triangle(s) in homogeneous space.
  **/
  void clip_triangle(const Triangle_gl &triangle_in,
                     std::vector<Triangle_gl> &triangles_out);
  /**
  Clip triangle (`v1`-`v2`-`v3`) in homogeneous space.
  Assume each vertex has homogeneous coordinate (x,y,z,w), then clip
  points outside -w <= x, y, z <= +w.
  @note: For detailed explanation of how to do clipping in homogeneous space,
  see: "How to clip in homogeneous space?" in "doc/graphics_pipeline.md".
  @param v1, v2, v3: Input triangle vertices in homogeneous space.
  @param clip_axis: Clip axis (0=x, 1=y, 2=z).
  @param clip_sign: Clip sign (+1 or -1). +1 means clip outside +w, -1 means
  clip outside -w.
  @param q1, q2, q3, q4: Output triangle vertices.
    - If one triangle is produced, then (`q1`-`q2`-`q3`) represents the new
  triangle.
    - If two triangles are produced, then (`q1`-`q2`-`q3`) represents the first
  triangle, (`q1`-`q3`-`q4`) represents the second triangle.
  @param n_tri: Number of triangle(s) produced after clipping.
  **/
  void clip_triangle(const Vertex_gl &v1, const Vertex_gl &v2,
                     const Vertex_gl &v3, const int clip_axis,
                     const int clip_sign, Vertex_gl &q1, Vertex_gl &q2,
                     Vertex_gl &q3, Vertex_gl &q4, int &n_tri);
  /**
  Clip segment A-B in homogeneous space. This is an auxiliary function for
  clip_triangle(...).
  @param A, B: Point A(Ax,Ay,Az,Aw) and B(Bx,By,Bz,Bw) in homogeneous space.
  @param clip_axis, clip_sign: The same with clip_triangle(...).
  @param t: Output interpolation weight between A (when t=0) and B (when t=1).
  The intersection point C can be caluclated as follows: C = (1-t)A + tB;
  **/
  void clip_segment(const Vertex_gl &A, const Vertex_gl &B, const int clip_axis,
                    const int clip_sign, double &t) {
    double A_i = A.gl_Position.i[clip_axis], A_w = A.gl_Position.w;
    double B_i = B.gl_Position.i[clip_axis], B_w = B.gl_Position.w;
    double S1, S2;
    if (clip_sign == +1) {
      S1 = A_i - A_w;
      S2 = B_i - B_w;
    } else {
      S1 = A_i + A_w;
      S2 = B_i + B_w;
    }
    t = (S1) / (S1 - S2);
  }

  /**
  @param p0, p1, p2: Window space coordinate (x,y,z,1/w), where (x,y) is the
  relative pixel location, z is the mapped NDC depth (see glFragDepth()), w is
  the real depth value in view space.
  @return: Minimum rectangle (x_s, y_s, x_e, y_e) that contains the triangle.
  **/
  Vec4 get_minimum_rect(const Vec4 &p0, const Vec4 &p1, const Vec4 &p2) {
    Vec2 bl, tr;
    bl.x = min(min(p0.x, p1.x), p2.x);
    bl.y = min(min(p0.y, p1.y), p2.y);
    tr.x = max(max(p0.x, p1.x), p2.x);
    tr.y = max(max(p0.y, p1.y), p2.y);
    return Vec4(bl.x, bl.y, tr.x, tr.y);
  }
  double edge_function(const Vec4 &p0, const Vec4 &p1, const Vec4 &p) {
    return (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y +
           (p0.x * p1.y - p0.y * p1.x);
  }
  /**
  Color convertion. Vec4 => RGBA8
  @param color: A Vec4 color (r,g,b,a), map value range [0.0, 1.0] to [0, 255],
  out of bound values will be clamped to 0 or 1 before conversion.
  **/
  void convert_Vec4_to_RGBA8(const Vec4 &color, uint8_t &R, uint8_t &G,
                             uint8_t &B, uint8_t &A) {
    R = uint8_t(min(max(int(color.x * 255.0), 0), 255));
    G = uint8_t(min(max(int(color.y * 255.0), 0), 255));
    B = uint8_t(min(max(int(color.z * 255.0), 0), 255));
    A = uint8_t(min(max(int(color.w * 255.0), 0), 255));
  }
  /**
  Surface read/write functionalities.
  @param p: Window coordinate (x, y), origin is at lower-left corner.
  @param fragment_out: Output of fragment shader.
  @param z: Depth value in window space [0, +1], 0/1: near/far.
  **/
  void write_surfaces(const Vec2 &p, const Vec4 &fragment_out, const double &z);

protected:
  struct {
    std::string projection_mode; /* "perspective" or "orthographic". */
    Vec3 projection_param;
    Vec3 position, look_at, up_dir;
  } eye;
  struct {
    /* not owned */
    Surface *color_surface;
    Surface *depth_surface;
  } surfaces;
  struct {
    /* vertices after vertex processing */
    std::vector<Vertex_gl> Vertices;
    /* geometry generated after vertex post-processing */
    std::vector<Triangle_gl> Triangles;
    /* fragment unit generated after rasterization */
    std::vector<Fragment_gl> Fragments;
  } ppl;

public:
  Pipeline();
  ~Pipeline();
};

};   // namespace ppl
