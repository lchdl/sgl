#pragma once
#include <stdint.h>

#include <string>
#include <vector>

#include "sgl_math.h"
#include "sgl_shader.h"
#include "sgl_texture.h"
#include "sgl_utils.h"
#include "sgl_model.h"

namespace sgl {

class Pipeline {
 public:
  /**
  Clear textures.
  @param color_texture: Color texture to be cleared.
  @param depth_texture: Depth texture to be cleared.
  @param clear_color: Color that will be filled to the color texture.
  @note: NULL value will be ignored.
  **/
  void clear_render_targets(
    Texture* color, 
    Texture* depth,
    const Vec4 &clear_color);
  /**
  Set vertex & fragment shaders.
  @note: NULL value will be ignored.
  **/
  void set_shaders(
      VS_func_t VS, 
      FS_func_t FS) {
    if (VS!=NULL) shaders.VS=VS;
    if (FS!=NULL) shaders.FS=FS;
  }
  /**
  Set render targets (color & depth textures).
  @note: NULL value will be ignored.
  **/
  void set_render_targets(
      Texture* color, 
      Texture* depth) {
    if (color!=NULL) targets.color=color;
    if (depth!=NULL) targets.depth=depth;
  }
  /**
  Enable/disable backface culling.
  **/
  void enable_backface_culling(bool state = true) {
    ppl.backface_culling = state;
  }
  void disable_backface_culling() {
    ppl.backface_culling = false;
  }
  /**
  Enable/disable depth test.
  **/
  void enable_depth_test(bool state = true) {
    ppl.do_depth_test = state;
  }
  void disable_depth_test() {
    ppl.do_depth_test = false;
  }
  /**
  Buffer manipulations.
  **/
  int32_t create_index_buffer();
  int32_t create_vertex_buffer();
  void fill_index_buffer(const int32_t& ibo, const IndexBuffer_t& buffer_data);
  void fill_vertex_buffer(const int32_t& vbo, const VertexBuffer_t& buffer_data);
  void delete_index_buffer(const int32_t& ibo);
  void delete_vertex_buffer(const int32_t& vbo);
  /** 
  Render triangles onto target textures.
  @param vertices: Vertex buffer object.
  @param indices: Index buffer object.
  @param uniforms: Uniform variables used by vertex and
    fragment shaders.
  **/
  virtual void draw(
    const VertexBuffer_t& vertices,
    const IndexBuffer_t& indices,
    const Uniforms& uniforms);
  virtual void draw(
    const int32_t& vbo,
    const int32_t& ibo,
    const Uniforms& uniforms
  );

 public:
  /**
  Set number of threads for rasterization.
  @param num_threads: Number of concurrent threads.
  **/
  void set_num_threads(const int& num_threads) {
    ppl.num_threads = num_threads;
  }

 protected:
  /**
  Internal class that is used in primitive assembly stage.
  Users do not need to care about it too much since it is just an simple
  aggregation of vertices that represent an assembled primitive.
  **/
  class Triangle_gl {
   public:
    Vertex_gl v[3];

   public:
    Triangle_gl() {}
    Triangle_gl(const Vertex_gl &v1, const Vertex_gl &v2, const Vertex_gl &v3) {
      this->v[0] = v1, this->v[1] = v2, this->v[2] = v3;
    }
  };

 protected:
  /**
  Stage I: Vertex Processing.
  @param vertex_buffer: The vertex buffer that is going to be processed.
  @param uniforms: The uniform variables given to the pipeline.
  @note: This function will invoke vertex shader, and all processed vertices
  will be stored into this->ppl.Vertices for further use.
  **/
  void vertex_processing(const VertexBuffer_t &vertex_buffer,
                         const Uniforms &uniforms);

  /**
  Stage II: Vertex Post-processing.
  @param index_buffer: The index buffer object that will tell us how the mesh is
  formed by using the vertex array.
  @note: After running post-processing, this->ppl.Triangles will be initialized
  properly and ready for the next step.
  **/
  void vertex_post_processing(const std::vector<int> &index_buffer);

  /**
  Stage III: Fragment Processing.
  @param uniforms: The uniform variables given to the pipeline.
  @param num_threads: The number of concurrent threads used for rasterization.
  @note: "MT" stands for "multi-threaded" version. 
         * If running in MT mode, OpenMP must be enabled.
  **/
  void fragment_processing(const Uniforms &uniforms);
  void fragment_processing_MT(const Uniforms &uniforms, const int &num_threads);

 protected:
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
  @param clip_sign: Clip sign (+1 or -1). +1 means clipping using +w, -1 means
  clipping using -w.
  @param q1, q2, q3, q4: Output triangle vertices.
    - If one triangle is produced, then (`q1`-`q2`-`q3`) represents the new
  triangle.
    - If two triangles are produced, then (`q1`-`q2`-`q3`) represents the first
  triangle, (`q1`-`q3`-`q4`) represents the second triangle.
  @param n_tri: Number of triangle(s) produced after clipping. Can be 0, 1, or 
  2 depending on different cases. If 0 is returned, then triangle is completely
  discarded.
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
  void clip_segment(const Vertex_gl &A, const Vertex_gl &B, 
                    const int clip_axis, const int clip_sign, double &t) {
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
  Get minimum rectangle in screen space that completely covers the whole triangle.
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
  /**
  Edge function. Determine which side the point p is at w.r.t. edge p0-p1.
  **/
  double edge(const Vec4 &p0, const Vec4 &p1, const Vec4 &p) {
    return (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y +
           (p0.x * p1.y - p0.y * p1.x);
  }
  /**
  Color convertion. Vec4 => RGBA8.
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
  void convert_RGBA8_to_uint32(const uint8_t& R, const uint8_t& G,
    const uint8_t& B, const uint8_t& A, uint32_t& RGBA) {
    RGBA = ((A << 24) | (B << 16) | (G << 8) | R);
  }
  /**
  Write final color data into targeted textures.
  @param p: Window coordinate (x, y), origin is at lower-left corner.
  @param color: Output color from the fragment shader.
  @param z: Depth value in window space [0, +1], 0/1: near/far.
  **/
  void write_render_targets(const Vec2 &p, const Vec4 &color, const double &z);

 protected:

  struct {
    Texture *color; /* not owned */
    Texture *depth; /* not owned */
  } targets; /* render targets */
  struct {
    std::vector<Vertex_gl> Vertices; /* vertices after vertex processing */
    std::vector<Triangle_gl> Triangles; /* geometry generated after vertex post-processing */
    int num_threads; /* number of cpu cores used when running the pipeline */
    bool backface_culling; /* enable/disable backface culling when rendering */
    bool do_depth_test; /* enable/disable depth test when rendering */
  } ppl; /* pipeline internal states and variables */
  struct {
    std::vector<VertexBuffer_t> VertexBuffers;
    std::vector<IndexBuffer_t> IndexBuffers;
  } buffers; /* cached buffers */
  struct {
    VS_func_t VS;
    FS_func_t FS;
  } shaders; /* shaders used by the pipeline */

  void _zero_init();

 public:
  Pipeline();
  Pipeline(VS_func_t VS, FS_func_t FS);
  ~Pipeline();
};

}; /* namespace sgl */
