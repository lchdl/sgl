#pragma once
#include <stdint.h>
#include <omp.h>

#include <string>
#include <vector>

#include "sgl_math.h"
#include "sgl_enums.h"
#include "sgl_shader.h"
#include "sgl_texture.h"
#include "sgl_utils.h"
#include "sgl_model.h"

namespace sgl {

/*
The core software rasterization pipeline is implemented as a template class,
allowing full customization by instantiating it with user-defined types. For
an example of how to use this, refer to 'tests/test_hello_world'.

Software Rasterizer Tutorial
* The OpenGL render pipeline overview:
  https://www.khronos.org/opengl/wiki/Rendering_Pipeline_Overview
* The model transformation matrix in OpenGL:
  https://learnopengl.com/Getting-started/Transformations
* The view matrix in OpenGL:
  http://www.songho.ca/opengl/gl_camera.html
* The perspective matrix in OpenGL:
  http://www.songho.ca/opengl/gl_projectionmatrix.html
* How to clip in homogeneous space?
  https://stackoverflow.com/questions/60910464/at-what-stage-is-clipping-performed-in-the-graphics-pipeline
* Simple and quick way to determine if a point is in a triangle, and all 
  things you need to know about barycentric interpolation:
  https://fgiesen.wordpress.com/2013/02/06/the-barycentric-conspirac/
* Perspective correct z-interpolation:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/visibility-problem-depth-buffer-depth-interpolation.html
* Perspective correct vertex attributes interpolation:
  https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes.html
* Fragment shader predefined outputs:
  https://www.khronos.org/opengl/wiki/Fragment_Shader
*/

template <typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
class Pipeline {
public:

  typedef std::vector<Vertex_t>     VertexBuffer_t;
  typedef std::vector<int32_t>       IndexBuffer_t;
  typedef std::vector<Fragment_t> FragmentBuffer_t;

  /** Clear **/
  void clear_render_target(const int& slot, const Vec4& clear_color);
  void clear_render_targets(const Vec4 &clear_color);
  void clear_cache();
  
  /** Set render targets **/
  void bind_render_target(const int& slot, Texture* texobj) { this->targets.out_texs[slot] = texobj; }
  void unbind_render_target(const int& slot) { this->targets.out_texs[slot] = NULL; }
  Texture* get_render_target(const int& slot) const { return this->targets.out_texs[slot]; }
  
  /** Enable/disable backface culling **/
  void set_backface_culling_state(bool state) { ppl.backface_culling = state; }
  bool get_backface_culling_state() const { return ppl.backface_culling; }
  
  /** Enable/disable depth test **/
  void set_depth_test_state(bool state) { ppl.do_depth_test = state; }
  bool get_depth_test_state() const { return ppl.do_depth_test; }

  /** Set draw mode **/
  void set_draw_mode(PipelineDrawMode draw_mode) { ppl.draw_mode = draw_mode; }
  PipelineDrawMode get_draw_mode() const { return ppl.draw_mode; }

  /** Set number of threads for rasterization **/
  void set_num_threads(const int& num_threads) { ppl.num_threads = num_threads; }
  int get_num_threads() const { return ppl.num_threads; }

  /** Render triangles onto target textures **/
  void draw(const Shader_t& shader, const VertexBuffer_t& vertices, const IndexBuffer_t& indices, const Uniforms_t& uniforms);

 protected:
  /**
  Stage I: Vertex Processing.
  @param vertex_buffer: The vertex buffer that is going to be processed.
  @param uniforms: The uniform variables given to the pipeline.
  @note: This function will invoke vertex shader, and all processed vertices
  will be stored into this->ppl.Vertices for further use.
  **/
  void vertex_processing(const Shader_t& shader, const VertexBuffer_t &vertex_buffer, const Uniforms_t& uniforms);

  /**
  Stage II: Vertex Post-processing.
  @param index_buffer: The index buffer object that will tell us how the mesh is
  formed by using the vertex array.
  @note: After running post-processing, this->ppl.Triangles will be initialized
  properly and ready for the next stage.
  **/
  void vertex_post_processing(const IndexBuffer_t& index_buffer);

  /**
  Stage III: Fragment Processing.
  @param uniforms: The uniform variables given to the pipeline.
  @param num_threads: The number of concurrent threads used for rasterization.
  @note: "MT" stands for "multi-threaded" version. 
         * If running in MT mode, OpenMP must be enabled.
  **/
  void fragment_processing_triangle(const Shader_t& shader, const Uniforms_t& uniforms);
  void fragment_processing_triangle_MT(const Shader_t& shader, const Uniforms_t& uniforms, const int &num_threads);
  void fragment_processing_wireframe(const Shader_t& shader, const Uniforms_t& uniforms);
  void fragment_processing_wireframe_MT(const Shader_t& shader, const Uniforms_t& uniforms, const int &num_threads);

 protected:
  class Triangle_t {
    /**
    Internal class that is used in primitive assembly stage.
    Users do not need to care about it too much since it is just an simple
    aggregation of vertices that represent an assembled primitive.
    **/
  public:
    Fragment_t v[3];

  public:
    Triangle_t() {}
    Triangle_t(const Fragment_t &v1, const Fragment_t &v2, const Fragment_t &v3) {
      this->v[0] = v1, this->v[1] = v2, this->v[2] = v3;
    };
  };
  typedef std::vector<Triangle_t> TriangleBuffer_t;

protected:
  /**
  Clip triangle in homogeneous space.
  @note: Assume each vertex has homogeneous coordinate (x,y,z,w), then clip points outside -w <= x, y, z <= +w.
  @param triangle_in: Input triangle in homogeneous space.
  @param triangles_out: Output triangle(s) in homogeneous space.
  **/
  void clip_triangle(const Triangle_t &triangle_in, TriangleBuffer_t& triangles_out);
  /**
  Clip triangle (`v1`-`v2`-`v3`) in homogeneous space.
  Assume each vertex has homogeneous coordinate (x,y,z,w), then clip
  points outside -w <= x, y, z <= +w.
  @note: For detailed explanation of how to do clipping in homogeneous space,
  see: "How to clip in homogeneous space?" in "doc/graphics_pipeline.md".
  @param v1, v2, v3: Input triangle vertices in homogeneous space.
  @param clip_axis: Clip axis (0=x, 1=y, 2=z).
  @param clip_sign: Clip sign (+1 or -1). +1 means clipping using +w, -1 means clipping using -w.
  @param q1, q2, q3, q4: Output triangle vertices.
    - If one triangle is produced, then (`q1`-`q2`-`q3`) represents the new triangle.
    - If two triangles are produced, then (`q1`-`q2`-`q3`) represents the first
  triangle, (`q1`-`q3`-`q4`) represents the second triangle.
  @param n_tri: Number of triangle(s) produced after clipping. Can be 0, 1, or 
  2 depending on different cases. If 0 is returned, then triangle is completely
  discarded.
  **/
  void clip_triangle(const Fragment_t &v1, const Fragment_t &v2,
                     const Fragment_t &v3, const int clip_axis,
                     const int clip_sign, Fragment_t &q1, Fragment_t &q2,
                     Fragment_t &q3, Fragment_t &q4, int &n_tri);
  /**
  Clip segment A-B in homogeneous space. This is an auxiliary function for
  clip_triangle(...).
  @param A, B: Point A(Ax,Ay,Az,Aw) and B(Bx,By,Bz,Bw) in homogeneous space.
  @param clip_axis, clip_sign: The same with clip_triangle(...).
  @param t: Output interpolation weight between A (when t=0) and B (when t=1).
  The intersection point C can be caluclated as follows: C = (1-t)A + tB;
  **/
  void clip_segment(const Fragment_t &A, const Fragment_t &B, 
                    const int clip_axis, const int clip_sign, double &t);
  /**
  Get minimum rectangle in screen space that completely covers the whole triangle.
  @param p0, p1, p2: Window space coordinate (x,y,z,1/w), where (x,y) is the
  relative pixel location, z is the mapped NDC depth (see glFragDepth()), w is
  the real depth value in view space.
  @return: Minimum rectangle (x_s, y_s, x_e, y_e) that contains the triangle.
  **/
  Vec4 get_minimum_rect(const Vec4 &p0, const Vec4 &p1, const Vec4 &p2);
  /**
  Edge function. Determine which side the point p is at w.r.t. edge p0-p1.
  **/
  double edge(const Vec4 &p0, const Vec4 &p1, const Vec4 &p);
  /**
  Write final color data into targeted textures.
  @param p: Window coordinate (x, y), origin is at lower-left corner.
  @param color: Output color from the fragment shader.
  @param z: Depth value in window space [0, +1], 0/1: near/far.
  **/
  void write_render_targets(const Vec2 &p, const FS_Outputs &fs_outs, const double &z);

  /**
  Internal functions for wireframe rendering.
  **/
  void _inner_interpolate(const Shader_t& shader, int x, int y, double q, 
    const Fragment_t& v1, const Fragment_t& v2, const Vec2 & iz, 
    const Uniforms_t& uniforms);
  void _bresenham_traversal(const Shader_t& shader, int x1, int y1, int x2, int y2, 
    const Fragment_t & v1, const Fragment_t & v2, const Vec2 & iz, 
    const Uniforms_t& uniforms);

 protected:
  struct {
    Texture* out_texs[MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS];
  } targets; /* render targets */
  struct {
    FragmentBuffer_t  Vertices; /* vertices after vertex processing */
    TriangleBuffer_t Triangles; /* geometry generated after vertex post-processing */
    int            num_threads; /* number of cpu cores used when running the pipeline */
    bool      backface_culling; /* enable/disable backface culling when rendering */
    bool         do_depth_test; /* enable/disable depth test when rendering */
    int       cur_render_width; /* cur_render_width/height will be properly set when
                                   a draw call is invoked based on bound textures in
                                   a frame buffer */
    int      cur_render_height;
    int     depth_texture_slot; /* which slot stores the depth texture, must be in range 
                                   [0, MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS) */
    PipelineDrawMode draw_mode; /* different draw modes will invoke different fragment 
                                   processing implementations */
  } ppl; /* pipeline internal states and variables */

  void _zero_init();

 public:
  Pipeline() { _zero_init(); }
  ~Pipeline() {}

};

inline Mat4x4 
get_view_matrix(Vec3 eye, Vec3 look_at, Vec3 up)
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

inline Mat4x4 
get_perspective_matrix(double aspect_ratio, double near, double far, double field_of_view) {
  /* aspect_ratio = w/h */
  double inv_aspect = 1.0 / aspect_ratio;
  double& n = near; /* near */
  double& f = far; /* far */
  double& fov = field_of_view;
  double l = -tan(fov / 2.0) * n; /* left */
  double r = -l; /* right */
  double t = inv_aspect * r; /* top */
  double b = -t; /* bottom */
  return Mat4x4(
    2 * n / (r - l), 0.0, (r + l) / (r - l), 0.0,
    0.0, 2 * n / (t - b), (t + b) / (t - b), 0.0,
    0.0, 0.0, -(f + n) / (f - n), -2 * f * n / (f - n),
    0.0, 0.0, -1.0, 0.0);
}

inline Mat4x4 
get_orthographic_matrix(double near, double far, double left, double right, double top, double bottom) {
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

inline Mat4x4 
get_orthographic_matrix(double near, double far, double width, double height) {
  return get_orthographic_matrix(near, far, -width * 0.5, width * 0.5, height * 0.5, -height * 0.5);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * Implementations below * * * * * * * * * * * * * * * * */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::_zero_init()
{
  for (int i = 0; i < MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS; i++) {
    targets.out_texs[i] = NULL;
  }
  ppl.num_threads = max(get_cpu_cores(), 1);
  ppl.backface_culling = true;
  ppl.do_depth_test = true;
  ppl.draw_mode = PipelineDrawMode_Triangle;
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clear_render_target(
  const int & slot, const Vec4 & clear_color)
{
  Texture* texture = targets.out_texs[slot];
  if (texture == NULL) return;
  texture->clear(clear_color);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clear_render_targets(const Vec4 & clear_color)
{
  for (int i = 0; i < MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS; i++)
    clear_render_target(i, clear_color);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clear_cache()
{
  ppl.Triangles.clear();
  ppl.Triangles.shrink_to_fit();
  ppl.Vertices.clear();
  ppl.Vertices.shrink_to_fit();
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::draw(
  const Shader_t& shader, const VertexBuffer_t & vertices, const IndexBuffer_t & indices, const Uniforms_t& uniforms)
{
  /*
  Initialize internal variables and check if frame buffer is complete.
  A complete frame buffer must satisfy:
  1) One and only one depth buffer if depth test is enabled, when depth test
     is disabled, the depth buffer is optional.
  2) At least one texture bound to one of the several output texture slots.
  3) Each bound texture must have the same width and height.
  */
  {
    bool is_ready = true;
    int num_depth_buffers = 0;
    ppl.cur_render_width = ppl.cur_render_height = -1;
    ppl.depth_texture_slot = -1;
    for (int i=0; i < MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS; i++) {
      if (targets.out_texs[i] == NULL) continue;
      /* set current render height and width parameters */
      if (ppl.cur_render_width < 0 || ppl.cur_render_height < 0) {
        ppl.cur_render_width = targets.out_texs[i]->get_width();
        ppl.cur_render_height = targets.out_texs[i]->get_height();
      }
      else {
        if (ppl.cur_render_width != targets.out_texs[i]->get_width() ||
          ppl.cur_render_height != targets.out_texs[i]->get_height()) {
          printf("Invalid frame buffer: different texture sizes detected! "
            "expected %dx%d, got %dx%d.\n", ppl.cur_render_width, ppl.cur_render_height,
            targets.out_texs[i]->get_width(), targets.out_texs[i]->get_height());
          is_ready = false;
          break;
        }
      }
      if (targets.out_texs[i]->get_texture_usage() == TextureUsage_DepthBuffer) {
        num_depth_buffers++;
        ppl.depth_texture_slot = i;
        if (targets.out_texs[i]->get_pixel_format() != PixelFormat_Float64) {
          is_ready = false;
        }
      }
    }
    /* check if pipeline is ready for render */
    if (ppl.num_threads < 1) {
      is_ready = false;
    }
    if (ppl.cur_render_width <= 0 || ppl.cur_render_height <= 0) {
      is_ready = false;
    }
    if (ppl.do_depth_test) {
      if (num_depth_buffers != 1 || ppl.depth_texture_slot < 0) {
        is_ready = false;
      }
    }
    if (!is_ready) {
      printf("Pipeline is not ready for render due to invalid parameter settings.\n");
      return;
    }
  }

  /* Clear cached data generated from previous call. */
  ppl.Vertices.clear();
  ppl.Triangles.clear();

  /* * * * * * * * * * * * * * * */
  /* Stage I: Vertex processing. */
  /* * * * * * * * * * * * * * * */
  vertex_processing(shader, vertices, uniforms);

  /* * * * * * * * * * * * * * * * * * */
  /* Stage II: Vertex post-processing. */
  /* * * * * * * * * * * * * * * * * * */
  vertex_post_processing(indices);

  /* * * * * * * * * * * * * * * * * * * * * * * * * */
  /* Stage III: Rasterization & fragment processing  */
  /* * * * * * * * * * * * * * * * * * * * * * * * * */
  if (ppl.draw_mode == PipelineDrawMode_Triangle) {
    if (ppl.num_threads > 1) {
      fragment_processing_triangle_MT(shader, uniforms, ppl.num_threads);
    }
    else {
      fragment_processing_triangle(shader, uniforms);
    }
  }
  else if (ppl.draw_mode == PipelineDrawMode_Wireframe) {
    if (ppl.num_threads > 1) {
      fragment_processing_wireframe_MT(shader, uniforms, ppl.num_threads);
    }
    else {
      fragment_processing_wireframe(shader, uniforms);
    }
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::vertex_processing(
  const Shader_t& shader, const VertexBuffer_t & vertex_buffer, const Uniforms_t & uniforms)
{
  for (uint32_t i_vert = 0; i_vert < vertex_buffer.size(); i_vert++) {
    Fragment_t vertex_out;
    Vec4 gl_Position;
    /* Map vertex from model local space to homogeneous clip space 
    and stores to "gl_Position". */
    shader.VS(uniforms, vertex_buffer[i_vert], vertex_out, gl_Position);
    vertex_out.gl_Position = gl_Position;
    ppl.Vertices.push_back(vertex_out);
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::vertex_post_processing(
  const IndexBuffer_t & index_buffer)
{
  for (uint32_t i_tri = 0; i_tri < index_buffer.size() / 3; i_tri++) {
    /* Step 2.1: Primitive assembly. */
    Triangle_t tri;
    tri.v[0] = ppl.Vertices[index_buffer[i_tri * 3]];
    tri.v[1] = ppl.Vertices[index_buffer[i_tri * 3 + 1]];
    tri.v[2] = ppl.Vertices[index_buffer[i_tri * 3 + 2]];
    /** Step 2.2: Clipping.
    @note: For detailed explanation of how to do clipping in homogeneous space,
    see: "How to clip in homogeneous space?" in "doc/graphics_pipeline.md".
    **/
    clip_triangle(tri, ppl.Triangles);
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::fragment_processing_triangle(
  const Shader_t& shader, const Uniforms_t& uniforms)
{
  for (uint32_t i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
    /* Step 3.1: Convert clip space to NDC space (perspective divide) */
    Triangle_t tri = ppl.Triangles[i_tri];
    Fragment_t &v0 = tri.v[0];
    Fragment_t &v1 = tri.v[1];
    Fragment_t &v2 = tri.v[2];
    const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w, 1.0 / v2.gl_Position.w);
    Vec3 p0_NDC = v0.gl_Position.xyz() * iz.i[0];
    Vec3 p1_NDC = v1.gl_Position.xyz() * iz.i[1];
    Vec3 p2_NDC = v2.gl_Position.xyz() * iz.i[2];
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
    const double render_width = ppl.cur_render_width;
    const double render_height = ppl.cur_render_height;
    const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
    const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
    const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
    const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
    double area = edge(p0, p1, p2);
    if (isnan(area) || isinf(area)) continue; /* Ignore invalid triangles. */
    if (area < 0.0 && ppl.backface_culling) continue; /* Backface culling. */
    /** @note: p0, p1, p2 are actually gl_FragCoord. **/
    /* Step 3.3: Rasterization. */
    Vec4 rect = get_minimum_rect(p0, p1, p2);
    /* precomupte: divide by real z */
    v0 *= iz.i[0];
    v1 *= iz.i[1];
    v2 *= iz.i[2];
    Vec4 p;
    for (p.y = floor(rect.i[1]) + 0.5; p.y < rect.i[3]; p.y += 1.0) {
      for (p.x = floor(rect.i[0]) + 0.5; p.x < rect.i[2]; p.x += 1.0) {
        /**
        @note: here the winding order is important,
        and w_i are calculated in window space
        **/
        Vec3 w = Vec3(edge(p1, p2, p), edge(p2, p0, p), edge(p0, p1, p));
        /* discard pixel if it is outside the triangle area */
        bool all_pos = (w.i[0] >= 0.0 && w.i[1] >= 0.0 && w.i[2] >= 0.0);
        bool all_neg = (w.i[0] <= 0.0 && w.i[1] <= 0.0 && w.i[2] <= 0.0);
        if (!all_pos && !all_neg) continue;
        /* interpolate vertex */
        w /= area;
        Fragment_t v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
        double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
        v_lerp *= z_real;
        /* Step 3.4: Assemble fragment and render pixel. */
        Fragment_t& fragment = v_lerp;
        /*
        v_lerp.gl_Position.z / v_lerp.gl_Position.w is the depth value in NDC
        space, which is in range [-1, +1], then we need to map it to [0, +1].

        * Although OpenGL's depth range is [-1, +1], but if you want to read the
          depth value from a depth texture, the value is further normalized to
          [0, +1]. So here for convenience we directly convert it to [0, +1]
          because reading from depth buffer is rather common in graphics
          programming.
        */
        double gl_FragDepth = ((v_lerp.gl_Position.z / v_lerp.gl_Position.w) + 1.0) * 0.5;
        Vec4 gl_FragCoord = Vec4(p.x, p.y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
        FS_Outputs fs_outs;
        bool is_discarded = false;
        shader.FS(uniforms, fragment, gl_FragCoord, fs_outs, is_discarded, gl_FragDepth);
        /* Step 3.5: Fragment processing */
        if (!is_discarded) {
          write_render_targets(gl_FragCoord.xy(), fs_outs, gl_FragDepth);
        }
      }
    }
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::fragment_processing_triangle_MT(
  const Shader_t& shader, const Uniforms_t& uniforms, const int & num_threads)
{
#pragma omp parallel for num_threads(num_threads)
  for (int thread_id = 0; thread_id < num_threads; thread_id++) {
    /**
    @note: interlaced rendering in MT mode. For example, if 4 threads (0~3)
    are used for rasterization, then:
    - thread 0 will only render y (rows) = 0, 4, 8, 12, ...
    - thread 1 will only render y (rows) = 1, 5, 9, 13, ...
    - thread 2 will only render y (rows) = 2, 6, 10, 14, ...
    - thread 3 will only render y (rows) = 3, 7, 11, 15, ...
    This is a way to achieve decent workload balance among workers.
    **/
    for (uint32_t i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
      /* Step 3.1: Convert clip space to NDC space (perspective divide) */
      Triangle_t tri = ppl.Triangles[i_tri];
      Fragment_t &v0 = tri.v[0];
      Fragment_t &v1 = tri.v[1];
      Fragment_t &v2 = tri.v[2];
      const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w, 1.0 / v2.gl_Position.w);
      Vec3 p0_NDC = v0.gl_Position.xyz() * iz.i[0];
      Vec3 p1_NDC = v1.gl_Position.xyz() * iz.i[1];
      Vec3 p2_NDC = v2.gl_Position.xyz() * iz.i[2];
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
      const double render_width = ppl.cur_render_width;
      const double render_height = ppl.cur_render_height;
      const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
      const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
      const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
      const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
      double area = edge(p0, p1, p2);
      if (isnan(area) || isinf(area)) continue; /* Ignore invalid triangles. */
      if (area < 0.0 && ppl.backface_culling) continue; /* Backface culling. */
      /** @note: p0, p1, p2 are actually gl_FragCoord. **/
      /* Step 3.3: Rasterization. */
      Vec4 rect = get_minimum_rect(p0, p1, p2);
      /* precomupte: divide by real z */
      v0 *= iz.i[0];
      v1 *= iz.i[1];
      v2 *= iz.i[2];
      Vec4 p;
      int y_base = num_threads * int(int(rect.i[1]) / num_threads);
      for (p.y = double(y_base) + 0.5 + double(thread_id); p.y < rect.i[3]; p.y += double(num_threads)) {
        for (p.x = floor(rect.i[0]) + 0.5; p.x < rect.i[2]; p.x += 1.0) {
          /**
          @note: here the winding order is important,
          and w_i are calculated in window space
          **/
          Vec3 w = Vec3(edge(p1, p2, p), edge(p2, p0, p), edge(p0, p1, p));
          /* discard pixel if it is outside the triangle area */
          bool all_pos = (w.i[0] >= 0.0 && w.i[1] >= 0.0 && w.i[2] >= 0.0);
          bool all_neg = (w.i[0] <= 0.0 && w.i[1] <= 0.0 && w.i[2] <= 0.0);
          if (!all_pos && !all_neg) continue;
          /* interpolate vertex */
          w /= area;
          Fragment_t v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
          double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
          v_lerp *= z_real;
          /* Step 3.4: Assemble fragment and render pixel. */
          Fragment_t& fragment = v_lerp;
          /*
          v_lerp.gl_Position.z / v_lerp.gl_Position.w is the depth value in NDC
          space, which is in range [-1, +1], then we need to map it to [0, +1].

          * Although OpenGL's depth range is [-1, +1], but if you want to read the
            depth value from a depth texture, the value is further normalized to
            [0, +1]. So here for convenience we directly convert it to [0, +1]
            because reading from depth buffer is rather common in graphics
            programming.
          */
          double gl_FragDepth = ((v_lerp.gl_Position.z / v_lerp.gl_Position.w) + 1.0) * 0.5;
          Vec4 gl_FragCoord = Vec4(p.x, p.y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
          FS_Outputs fs_outs;
          bool is_discarded = false;
          shader.FS(uniforms, fragment, gl_FragCoord, fs_outs, is_discarded, gl_FragDepth);
          /* Step 3.5: Fragment processing */
          if (!is_discarded) {
            write_render_targets(gl_FragCoord.xy(), fs_outs, gl_FragDepth);
          }
        }
      }
    }
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::fragment_processing_wireframe(
  const Shader_t & shader, const Uniforms_t & uniforms)
{
  for (uint32_t i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
    /* Step 3.1: Convert clip space to NDC space (perspective divide) */
    Triangle_t tri = ppl.Triangles[i_tri];
    Fragment_t &v0 = tri.v[0];
    Fragment_t &v1 = tri.v[1];
    Fragment_t &v2 = tri.v[2];
    const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w, 1.0 / v2.gl_Position.w);
    Vec3 p0_NDC = v0.gl_Position.xyz() * iz.i[0];
    Vec3 p1_NDC = v1.gl_Position.xyz() * iz.i[1];
    Vec3 p2_NDC = v2.gl_Position.xyz() * iz.i[2];
    /* Step 3.2: Convert NDC space to window space */
    const double render_width = ppl.cur_render_width;
    const double render_height = ppl.cur_render_height;
    const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
    const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
    const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
    const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
    double area = edge(p0, p1, p2);
    if (isnan(area) || isinf(area)) continue; /* Ignore invalid triangles. */
    if (area < 0.0 && ppl.backface_culling) continue; /* Backface culling. */
    /** @note: p0, p1, p2 are actually gl_FragCoord. **/
    /* Step 3.3: Rasterization. */
    /* precomupte: divide by real z */
    v0 *= iz.i[0];
    v1 *= iz.i[1];
    v2 *= iz.i[2];
    IVec2 ip0 = IVec2(int(p0.x), int(p0.y));
    IVec2 ip1 = IVec2(int(p1.x), int(p1.y));
    IVec2 ip2 = IVec2(int(p2.x), int(p2.y));
    _bresenham_traversal(shader, ip0.x, ip0.y, ip1.x, ip1.y, v0, v1, Vec2(iz.x, iz.y), uniforms);
    _bresenham_traversal(shader, ip1.x, ip1.y, ip2.x, ip2.y, v1, v2, Vec2(iz.x, iz.y), uniforms);
    _bresenham_traversal(shader, ip2.x, ip2.y, ip0.x, ip0.y, v2, v0, Vec2(iz.x, iz.y), uniforms);
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::fragment_processing_wireframe_MT(
  const Shader_t & shader, const Uniforms_t & uniforms, const int & num_threads)
{
#pragma omp parallel for num_threads(num_threads)
  for (int thread_id = 0; thread_id < num_threads; thread_id++) {
    for (uint32_t i_tri = thread_id; i_tri < ppl.Triangles.size(); i_tri+=num_threads) {
      /* Step 3.1: Convert clip space to NDC space (perspective divide) */
      Triangle_t tri = ppl.Triangles[i_tri];
      Fragment_t &v0 = tri.v[0];
      Fragment_t &v1 = tri.v[1];
      Fragment_t &v2 = tri.v[2];
      const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w, 1.0 / v2.gl_Position.w);
      Vec3 p0_NDC = v0.gl_Position.xyz() * iz.i[0];
      Vec3 p1_NDC = v1.gl_Position.xyz() * iz.i[1];
      Vec3 p2_NDC = v2.gl_Position.xyz() * iz.i[2];
      /* Step 3.2: Convert NDC space to window space */
      const double render_width = ppl.cur_render_width;
      const double render_height = ppl.cur_render_height;
      const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
      const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
      const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
      const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
      double area = edge(p0, p1, p2);
      if (isnan(area) || isinf(area)) continue; /* Ignore invalid triangles. */
      if (area < 0.0 && ppl.backface_culling) continue; /* Backface culling. */
      /** @note: p0, p1, p2 are actually gl_FragCoord. **/
      /* Step 3.3: Rasterization. */
      /* precomupte: divide by real z */
      v0 *= iz.i[0];
      v1 *= iz.i[1];
      v2 *= iz.i[2];
      IVec2 ip0 = IVec2(int(p0.x), int(p0.y));
      IVec2 ip1 = IVec2(int(p1.x), int(p1.y));
      IVec2 ip2 = IVec2(int(p2.x), int(p2.y));
      _bresenham_traversal(shader, ip0.x, ip0.y, ip1.x, ip1.y, v0, v1, Vec2(iz.x, iz.y), uniforms);
      _bresenham_traversal(shader, ip1.x, ip1.y, ip2.x, ip2.y, v1, v2, Vec2(iz.x, iz.y), uniforms);
      _bresenham_traversal(shader, ip2.x, ip2.y, ip0.x, ip0.y, v2, v0, Vec2(iz.x, iz.y), uniforms);
    }
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clip_triangle(
  const Triangle_t & triangle_in, TriangleBuffer_t & triangles_out)
{
  TriangleBuffer_t Q0, Q1;
  TriangleBuffer_t *Qcur = &Q0, *Qnext = &Q1, *Qtemp = NULL;
  Fragment_t q[4];
  const int clip_signs[2] ={ +1, -1 };
  /*
  Clip triangle using homogeneous cube -w <= x,y,z <= +w
  Since this cube has six faces, for each triangle we need to clip
  it six times, resulting in zero, one, or multiple triangles. We
  then return these clipped triangles to the caller.
  */
  Qcur->push_back(triangle_in);
  for (uint32_t clip_axis = 0; clip_axis < 3; clip_axis++) {
    for (uint32_t i_clip = 0; i_clip < 2; i_clip++) {
      const int clip_sign = clip_signs[i_clip];
      for (uint32_t i_tri = 0; i_tri < Qcur->size(); i_tri++) {
        Triangle_t &tri = (*Qcur)[i_tri];
        int n_tri;
        clip_triangle(tri.v[0], tri.v[1], tri.v[2],
          clip_axis, clip_sign, q[0], q[1], q[2], q[3], n_tri);
        if (n_tri == 1) {
          Qnext->push_back(Triangle_t(q[0], q[1], q[2]));
        }
        else if (n_tri == 2) {
          Qnext->push_back(Triangle_t(q[0], q[1], q[2]));
          Qnext->push_back(Triangle_t(q[0], q[2], q[3]));
        }
      }
      /* swap `Qcur` and `Qnext` for the next iteration */
      Qcur->clear();
      Qtemp = Qcur;
      Qcur = Qnext;
      Qnext = Qtemp;
    }
  }
  for (uint32_t i_tri = 0; i_tri < Qcur->size(); i_tri++)
    triangles_out.push_back((*Qcur)[i_tri]);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clip_triangle(
  const Fragment_t & v1, const Fragment_t & v2, const Fragment_t & v3, const int clip_axis, 
  const int clip_sign, Fragment_t & q1, Fragment_t & q2, Fragment_t & q3, Fragment_t & q4, 
  int & n_tri)
{
  int p1_sign, p2_sign, p3_sign;
  if (clip_sign == +1) {
    /* use <= instead of <, if a point lies on the clip plane we don't need to clip it. */
    p1_sign = (v1.gl_Position.i[clip_axis] <= v1.gl_Position.w) ? +1 : -1;
    p2_sign = (v2.gl_Position.i[clip_axis] <= v2.gl_Position.w) ? +1 : -1;
    p3_sign = (v3.gl_Position.i[clip_axis] <= v3.gl_Position.w) ? +1 : -1;
  }
  else {
    /* use >= instead of >. */
    p1_sign = (v1.gl_Position.i[clip_axis] >= -v1.gl_Position.w) ? +1 : -1;
    p2_sign = (v2.gl_Position.i[clip_axis] >= -v2.gl_Position.w) ? +1 : -1;
    p3_sign = (v3.gl_Position.i[clip_axis] >= -v3.gl_Position.w) ? +1 : -1;
  }

  if (p1_sign < 0 && p2_sign < 0 && p3_sign < 0) {
    /* triangle is completely outside the clipping volume, simply discard it. */
    n_tri = 0;
  }
  else if (p1_sign > 0 && p2_sign > 0 && p3_sign > 0) {
    /* triangle is completely inside clipping volume, we don't need to do any
     * clipping operations */
    n_tri = 1;
    q1 = v1, q2 = v2, q3 = v3;
  }
  else {
    /* clipping is needed, check how many vertices are in the upper side of
    the plane, to obtain the number of newly generated triangles */
    n_tri = 0;
    if (p1_sign > 0)
      n_tri++;
    if (p2_sign > 0)
      n_tri++;
    if (p3_sign > 0)
      n_tri++;
    const Fragment_t *v[3];
    /* sort pointers, ensuring that the clipping plane always intersects with
    edge v[0]-v[1] and v[0]-v[2] */
    if (n_tri == 1) {
      /* ensure v[0] is always inside the clipping plane */
      if (p1_sign > 0) {
        v[0] = &v1, v[1] = &v2, v[2] = &v3;
      }
      else if (p2_sign > 0) {
        v[0] = &v2, v[1] = &v3, v[2] = &v1;
      }
      else {
        v[0] = &v3, v[1] = &v1, v[2] = &v2;
      }
    }
    else {
      /* ensure v[0] is always outside the clipping plane */
      if (p1_sign < 0) {
        v[0] = &v1, v[1] = &v2, v[2] = &v3;
      }
      else if (p2_sign < 0) {
        v[0] = &v2, v[1] = &v3, v[2] = &v1;
      }
      else {
        v[0] = &v3, v[1] = &v1, v[2] = &v2;
      }
    }
    /**
    Then, clip segments p0-p1 and p0-p2.

                ** How to perform clipping in homogeneous space **
    ----------------------------------------------------------------------------

    Assume we have two vertices A(A_x, A_y, A_z, A_w) and B(B_x, B_y, B_z, B_w),
    segment A-B will be clipped by a plane, assume the intersection is C, such
    that C = (1-t)A + tB, where 0 < t < 1.
    Then we have:
                              C_w = (1-t)*A_w + t*B_w.
    For the near plane (z-axis) clipping, we have:
                                     C_z = -C_w.
    Since C_z = (1-t)*A_z + t*B_z, then we also have:
                     -(1-t)*A_w - t*B_w = (1-t)*A_z + t*B_z.
    We can solve for scalar t:
                       t = (A_z+A_w) / ((A_z+A_w)-(B_z+B_w)).
    Similarly, we can solve scalar t for far plane clipping:
                       t = (A_z-A_w) / ((A_z-A_w)-(B_z-B_w)).
    Clipping with other axes is also the same. Just replace A_z to A_x or A_y
    and B_z to B_x or B_y.

    From: https://stackoverflow.com/questions/60910464/at-what-stage-is-clipping-performed-in-the-graphics-pipeline
    => The scalar t can also be used to interpolate all the associated vertex
    attributes for C. The linear interpolation is perfectly sufficient even in
    perspective distorted cases, because we are before the perspective divide
    here, were the whole perspective transformation is perfectly affine w.r.t.
    the 4D space we work in.
    **/
    double t[2];
    clip_segment(*v[0], *v[1], clip_axis, clip_sign, t[0]);
    clip_segment(*v[0], *v[2], clip_axis, clip_sign, t[1]);

    if (n_tri == 1) {
      q1 = *(v[0]);
      q2 = *(v[0]) * (1 - t[0]) + *(v[1]) * t[0];
      q3 = *(v[0]) * (1 - t[1]) + *(v[2]) * t[1];
    }
    else if (n_tri == 2) {
      q1 = *(v[1]), q2 = *(v[2]);
      q3 = *(v[0]) * (1 - t[1]) + *(v[2]) * t[1];
      q4 = *(v[0]) * (1 - t[0]) + *(v[1]) * t[0];
    }
    /* for the case when n_tri==0, the triangle is automatically discarded. */
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::clip_segment(
  const Fragment_t & A, const Fragment_t & B, const int clip_axis, const int clip_sign, double & t) 
{
  double A_i = A.gl_Position.i[clip_axis], A_w = A.gl_Position.w;
  double B_i = B.gl_Position.i[clip_axis], B_w = B.gl_Position.w;
  double S1, S2;
  if (clip_sign == +1) {
    S1 = A_i - A_w;
    S2 = B_i - B_w;
  }
  else {
    S1 = A_i + A_w;
    S2 = B_i + B_w;
  }
  t = (S1) / (S1 - S2);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline Vec4 Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::get_minimum_rect(
  const Vec4 & p0, const Vec4 & p1, const Vec4 & p2) 
{
  Vec2 bl, tr;
  bl.x = min(min(p0.x, p1.x), p2.x);
  bl.y = min(min(p0.y, p1.y), p2.y);
  tr.x = max(max(p0.x, p1.x), p2.x);
  tr.y = max(max(p0.y, p1.y), p2.y);
  return Vec4(bl.x, bl.y, tr.x, tr.y);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline double Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::edge(
  const Vec4 & p0, const Vec4 & p1, const Vec4 & p) 
{
  return (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y + (p0.x * p1.y - p0.y * p1.x);
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::write_render_targets(
  const Vec2 & p, const FS_Outputs & fs_outs, const double & z)
{
  int w = ppl.cur_render_width;
  int h = ppl.cur_render_height;
  int ix = int(p.x);
  int iy = h - 1 - int(p.y);
  if (ix < 0 || ix >= w || iy < 0 || iy >= h)
    return;
  /* here (ix,iy) is the final output pixel location in window space (origin is at the top-left corner of the screen). */
  int pixel_id = iy * w + ix;

  /* depth test */
  if (ppl.do_depth_test) {
    double *depths = (double *)this->targets.out_texs[ppl.depth_texture_slot]->get_pixel_data();
    double z_new = min(max(z, 0.0), 1.0);
    double z_orig = depths[pixel_id];
    if (z_new > z_orig)
      return;
    depths[pixel_id] = z_new;
  }

  /* write each color component to their corresponding texture slot */
  for (int i_slot=0; i_slot < MAX_FRAGMENT_SHADER_OUTPUT_COLOR_COMPONENTS; i_slot++) {
    if (targets.out_texs[i_slot] == NULL) continue; /* this slot does not link to any texture, skip */
    if (i_slot == ppl.depth_texture_slot) continue; /* skip depth buffer since we already processed it in above */
    const Vec4& color = fs_outs[i_slot];
    /*
    write this color component to the corresponding texture slot, but
    be aware that different texture formats will have different physical
    storage layout
    */
    if (targets.out_texs[i_slot]->get_pixel_format() == PixelFormat_BGRA8888 ||
      targets.out_texs[i_slot]->get_pixel_format() == PixelFormat_RGBA8888) {
      uint8_t R, G, B, A;
      uint32_t packed_32bit;
      convert_Vec4_color_to_RGBA_uint8(color, R, G, B, A);
      pack_RGBA8888_to_uint32(R, G, B, A, targets.out_texs[i_slot]->get_pixel_format(), packed_32bit);
      uint32_t *pixels = (uint32_t *)targets.out_texs[i_slot]->get_pixel_data();
      pixels[pixel_id] = packed_32bit;
    }
    else if (targets.out_texs[i_slot]->get_pixel_format() == PixelFormat_Float64) {
      /* we only select the first component of the Vec4 color (color.i[0]), other components are ignored */
      double data = color.i[0];
      double *pixels = (double *)targets.out_texs[i_slot]->get_pixel_data();
      pixels[pixel_id] = data;
    }
    else if (targets.out_texs[i_slot]->get_pixel_format() == PixelFormat_Float32) {
      /* we only select the first component of the Vec4 color (color.i[0]), other components are ignored */
      double data = color.i[0];
      float *pixels = (float *)targets.out_texs[i_slot]->get_pixel_data();
      pixels[pixel_id] = float(data);
    }
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void sgl::Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::_inner_interpolate(
  const Shader_t& shader, int x, int y, double q, const Fragment_t& v1, const Fragment_t& v2, 
  const Vec2 & iz, const Uniforms_t& uniforms)
{
  Vec2 w = Vec2(q, 1.0 - q);
  Fragment_t v_lerp = v1 * w.i[0] + v2 * w.i[1];
  double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1]);
  v_lerp *= z_real;
  Fragment_t& fragment = v_lerp;

  double gl_FragDepth = ((v_lerp.gl_Position.z / v_lerp.gl_Position.w) + 1.0) * 0.5;
  Vec4 gl_FragCoord = Vec4(x, y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
  FS_Outputs fs_outs;
  bool is_discarded = false;
  shader.FS(uniforms, fragment, gl_FragCoord, fs_outs, is_discarded, gl_FragDepth);
  if (!is_discarded) {
    write_render_targets(gl_FragCoord.xy(), fs_outs, gl_FragDepth);
  }
}

template<typename Uniforms_t, typename Vertex_t, typename Fragment_t, typename Shader_t>
inline void Pipeline<Uniforms_t, Vertex_t, Fragment_t, Shader_t>::_bresenham_traversal(
  const Shader_t & shader, int x1, int y1, int x2, int y2, const Fragment_t & v1, 
  const Fragment_t & v2, const Vec2 & iz, const Uniforms_t & uniforms)
{
  /* NOTE: internal drawing function, do not call it directly. */
  int dx, dy;
  int x, y;
  int epsilon = 0;
  int Dx = x2 - x1;
  int Dy = y1 - y2;
  Dx > 0 ? dx = +1 : dx = -1;
  Dy > 0 ? dy = -1 : dy = +1;
  Dx = ::abs(Dx), Dy = ::abs(Dy);
  if (Dx > Dy) {
    y = y1;
    for (x = x1; x != x2; x += dx) {
      /* process (x, y) here */
      double q = double(x2 - x) / double(Dx);
      _inner_interpolate(shader, x, y, q, v1, v2, iz, uniforms);
      /* prepare for next iteration */
      epsilon += Dy;
      if ((epsilon << 1) > Dx) {
        y += dy;
        epsilon -= Dx;
      }
    }
  }
  else {
    x = x1;
    for (y = y1; y != y2; y += dy) {
      /* process (x, y) here */
      double q = double(y2 - y) / double(Dy);
      _inner_interpolate(shader, x, y, q, v1, v2, iz, uniforms);
      /* prepare for next iteration */
      epsilon += Dx;
      if ((epsilon << 1) > Dy) {
        epsilon -= Dy;
        x += dx;
      }
    }
  }
}

}; /* namespace sgl */
