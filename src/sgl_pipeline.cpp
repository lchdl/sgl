#include "sgl_pipeline.h"

#include <omp.h>

namespace sgl {

void Pipeline::_zero_init()
{
  targets.color = NULL;
  targets.depth = NULL;
  shaders.VS = NULL;
  shaders.FS = NULL;
  ppl.num_threads = max(get_cpu_cores(), 1);
  ppl.backface_culling = true;
  ppl.do_depth_test = true;
}

Pipeline::Pipeline() {
  _zero_init();
}
Pipeline::Pipeline(VS_func_t VS, FS_func_t FS)
{
  _zero_init();
  shaders.VS = VS;
  shaders.FS = FS;
}
Pipeline::~Pipeline() {}

int32_t Pipeline::create_index_buffer()
{
  for (uint32_t i = 0; i < buffers.IndexBuffers.size(); i++) {
    if (buffers.IndexBuffers[i].size() == 0) {
      return (int32_t)i;
    }
  }
  buffers.IndexBuffers.push_back(IndexBuffer_t());
  return (int32_t)buffers.IndexBuffers.size() - 1;
}

int32_t Pipeline::create_vertex_buffer()
{
  for (uint32_t i = 0; i < buffers.VertexBuffers.size(); i++) {
    if (buffers.VertexBuffers[i].size() == 0) {
      return (int32_t)i;
    }
  }
  buffers.VertexBuffers.push_back(VertexBuffer_t());
  return (int32_t)buffers.VertexBuffers.size() - 1;
}

void Pipeline::fill_index_buffer(const int32_t & ibo, const IndexBuffer_t & buffer_data)
{
  IndexBuffer_t& buffer = buffers.IndexBuffers[ibo];
  buffer.clear();
  buffer.shrink_to_fit();
  buffer = buffer_data;
}

void Pipeline::fill_vertex_buffer(const int32_t & vbo, const VertexBuffer_t & buffer_data)
{
  VertexBuffer_t& buffer = buffers.VertexBuffers[vbo];
  buffer.clear();
  buffer.shrink_to_fit();
  buffer = buffer_data;
}

void Pipeline::delete_index_buffer(const int32_t & ibo)
{
  buffers.IndexBuffers[ibo].clear();
  buffers.IndexBuffers[ibo].shrink_to_fit();
}

void Pipeline::delete_vertex_buffer(const int32_t & vbo)
{
  buffers.IndexBuffers[vbo].clear();
  buffers.IndexBuffers[vbo].shrink_to_fit();
}

void Pipeline::draw(
  const VertexBuffer_t& vertices,
  const IndexBuffer_t& indices,
  const Uniforms& uniforms) 
{
  if (shaders.VS == NULL || shaders.FS == NULL)
    return;

  /* Clear cached data generated from previous call. */
  ppl.Vertices.clear();
  ppl.Triangles.clear();

  /* Stage I: Vertex processing. */
  vertex_processing(vertices, uniforms);

  /* Stage II: Vertex post-processing. */
  vertex_post_processing(indices);

  /* Step III: Rasterization & fragment processing */
  fragment_processing_MT(uniforms, ppl.num_threads);
}

void Pipeline::draw(
  const int32_t & vbo, 
  const int32_t & ibo, 
  const Uniforms& uniforms)
{
  this->draw(
    buffers.VertexBuffers[vbo], 
    buffers.IndexBuffers[ibo], 
    uniforms
  );
}

void
Pipeline::vertex_processing(const VertexBuffer_t &vertex_buffer,
                            const Uniforms &uniforms) {
  for (uint32_t i_vert = 0; i_vert < vertex_buffer.size(); i_vert++) {
    Vertex_gl vertex_out;
    /* Map vertex from model local space to homogeneous clip space and stores to
    "gl_Position". */
    shaders.VS(uniforms, vertex_buffer[i_vert], vertex_out);
    ppl.Vertices.push_back(vertex_out);
  }
}

void
Pipeline::vertex_post_processing(const std::vector<int> &index_buffer) {
  for (uint32_t i_tri = 0; i_tri < index_buffer.size() / 3; i_tri++) {
    /* Step 2.1: Primitive assembly. */
    Triangle_gl tri_gl;
    tri_gl.v[0] = ppl.Vertices[index_buffer[i_tri * 3]];
    tri_gl.v[1] = ppl.Vertices[index_buffer[i_tri * 3 + 1]];
    tri_gl.v[2] = ppl.Vertices[index_buffer[i_tri * 3 + 2]];
    /** Step 2.2: Clipping.
    @note: For detailed explanation of how to do clipping in homogeneous space,
    see: "How to clip in homogeneous space?" in "doc/graphics_pipeline.md".
    **/
    clip_triangle(tri_gl, ppl.Triangles);
  }
}

void
Pipeline::fragment_processing(const Uniforms &uniforms) {
  for (uint32_t i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
    /* Step 3.1: Convert clip space to NDC space (perspective divide) */
    Triangle_gl tri_gl = ppl.Triangles[i_tri];
    Vertex_gl &v0 = tri_gl.v[0];
    Vertex_gl &v1 = tri_gl.v[1];
    Vertex_gl &v2 = tri_gl.v[2];
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
    const double render_width = double(this->targets.color->w);
    const double render_height = double(this->targets.color->h);
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
        Vertex_gl v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
        double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
        v_lerp *= z_real;
        /* Step 3.4: Assemble fragment and render pixel. */
        Fragment_gl fragment;
        assemble_fragment(v_lerp, fragment);
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
        fragment.gl_FragCoord = Vec4(p.x, p.y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
        Vec4 color_out;
        bool is_discarded = false;
        shaders.FS(uniforms, fragment, color_out, is_discarded, gl_FragDepth);
        /* Step 3.5: Fragment processing */
        if (!is_discarded) {
          write_render_targets(fragment.gl_FragCoord.xy(), color_out,
            gl_FragDepth);
        }
      }
    }
  }
}

void
Pipeline::fragment_processing_MT(const Uniforms &uniforms,
                                 const int &num_threads) {
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
      Triangle_gl tri_gl = ppl.Triangles[i_tri];
      Vertex_gl &v0 = tri_gl.v[0];
      Vertex_gl &v1 = tri_gl.v[1];
      Vertex_gl &v2 = tri_gl.v[2];
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
      const double render_width = double(this->targets.color->w);
      const double render_height = double(this->targets.color->h);
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
          Vertex_gl v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
          double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
          v_lerp *= z_real;
          /* Step 3.4: Assemble fragment and render pixel. */
          Fragment_gl fragment;
          assemble_fragment(v_lerp, fragment);
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
          fragment.gl_FragCoord = Vec4(p.x, p.y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
          Vec4 color_out;
          bool is_discarded = false;
          shaders.FS(uniforms, fragment, color_out, is_discarded, gl_FragDepth);
          /* Step 3.5: Fragment processing */
          if (!is_discarded) {
            write_render_targets(fragment.gl_FragCoord.xy(), color_out,
              gl_FragDepth);
          }
        }
      }
    }
  }
}

void
Pipeline::write_render_targets(const Vec2 &p, const Vec4 &color, const double &z) {
  int w = this->targets.color->w;
  int h = this->targets.color->h;
  int ix = int(p.x);
  int iy = h - 1 - int(p.y);
  if (ix < 0 || ix >= w || iy < 0 || iy >= h)
    return;
  /* here (ix,iy) is the final output pixel location in window space 
  (origin is at the top-left corner of the screen). */
  int pixel_id = iy * w + ix;
  /* depth test */
  double *depths = (double *) this->targets.depth->pixels;
  double z_new = min(max(z, 0.0), 1.0);
  double z_orig = depths[pixel_id];
  if (z_new > z_orig && ppl.do_depth_test)
    return;
  if (ppl.do_depth_test)
    depths[pixel_id] = z_new;
  uint8_t R, G, B, A;
  uint32_t packed_32bit;
  unpack_color_to_unsigned_RGBA(color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, this->targets.color->format, packed_32bit);
  uint32_t *pixels = (uint32_t *) this->targets.color->pixels;
  pixels[pixel_id] = packed_32bit;
}

void
Pipeline::clip_triangle(const Triangle_gl &triangle_in,
                        std::vector<Triangle_gl> &triangles_out) {
  std::vector<Triangle_gl> Q0, Q1;
  std::vector<Triangle_gl> *Qcur = &Q0, *Qnext = &Q1, *Qtemp = NULL;
  Qcur->push_back(triangle_in);
  Vertex_gl q[4];
  const int clip_signs[2] = {+1, -1};
  for (uint32_t clip_axis = 0; clip_axis < 3; clip_axis++) {
    for (uint32_t i_clip = 0; i_clip < 2; i_clip++) {
      const int clip_sign = clip_signs[i_clip];
      for (uint32_t i_tri = 0; i_tri < Qcur->size(); i_tri++) {
        Triangle_gl &tri = (*Qcur)[i_tri];
        int n_tri;
        clip_triangle(tri.v[0], tri.v[1], tri.v[2], 
          clip_axis, clip_sign, q[0], q[1], q[2], q[3], n_tri);
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
      Qcur = Qnext;
      Qnext = Qtemp;
    }
  }
  for (uint32_t i_tri = 0; i_tri < Qcur->size(); i_tri++)
    triangles_out.push_back((*Qcur)[i_tri]);
}

void
Pipeline::clip_triangle(const Vertex_gl &v1, const Vertex_gl &v2,
                        const Vertex_gl &v3, const int clip_axis,
                        const int clip_sign, Vertex_gl &q1, Vertex_gl &q2,
                        Vertex_gl &q3, Vertex_gl &q4, int &n_tri) {
  int p1_sign, p2_sign, p3_sign;
  if (clip_sign == +1) {
    /* use <= instead of <, if a point lies on the clip plane we don't need to
     * clip it. */
    p1_sign = (v1.gl_Position.i[clip_axis] <= v1.gl_Position.w) ? +1 : -1;
    p2_sign = (v2.gl_Position.i[clip_axis] <= v2.gl_Position.w) ? +1 : -1;
    p3_sign = (v3.gl_Position.i[clip_axis] <= v3.gl_Position.w) ? +1 : -1;
  } else {
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
      q2 = Vertex_gl::lerp(*v[0], *v[1], t[0]);
      q3 = Vertex_gl::lerp(*v[0], *v[2], t[1]);
    } 
    else if (n_tri == 2) {
      q1 = *(v[1]), q2 = *(v[2]);
      q3 = Vertex_gl::lerp(*v[0], *v[2], t[1]);
      q4 = Vertex_gl::lerp(*v[0], *v[1], t[0]);
    }
    /* for the case when n_tri==0, the triangle is automatically discarded. */
  }
}
void
Pipeline::clear_render_targets(
  Texture* color, 
  Texture* depth,
  const Vec4 &clear_color)
{ 
  uint8_t R, G, B, A;
  uint32_t packed_32bit;
  unpack_color_to_unsigned_RGBA(clear_color, R, G, B, A);
  pack_RGBA8888_to_uint32(R, G, B, A, this->targets.color->format, packed_32bit);

  if (color != NULL) {
    int n_pixels = color->w * color->h;
    uint32_t *pixels = (uint32_t *) color->pixels;
    for (int i = 0; i < n_pixels; i++) 
      pixels[i] = packed_32bit;
  }
  if (depth != NULL) {
    int n_pixels = depth->w * depth->h;
    double *pixels = (double *) depth->pixels;
    for (int i = 0; i < n_pixels; i++)
      pixels[i] = 1.0;
  }
}

void WireframePipeline::draw(
  const std::vector<Vertex>& vertices,
  const std::vector<int32_t>& indices,
  const Uniforms & uniforms)
{
  ppl.Vertices.clear();
  ppl.Triangles.clear();

  vertex_processing(vertices, uniforms);
  vertex_post_processing(indices);
  fragment_processing(uniforms);
}
void WireframePipeline::draw(
  const int32_t & vbo,
  const int32_t & ibo,
  const Uniforms & uniforms)
{
  this->draw(
    buffers.VertexBuffers[vbo],
    buffers.IndexBuffers[ibo],
    uniforms
  );
}

void WireframePipeline::fragment_processing(const Uniforms & uniforms)
{
  for (uint32_t i_tri = 0; i_tri < ppl.Triangles.size(); i_tri++) {
    /* Step 3.1: Convert clip space to NDC space (perspective divide) */
    Triangle_gl tri_gl = ppl.Triangles[i_tri];
    Vertex_gl &v0 = tri_gl.v[0];
    Vertex_gl &v1 = tri_gl.v[1];
    Vertex_gl &v2 = tri_gl.v[2];
    const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w, 1.0 / v2.gl_Position.w);
    Vec3 p0_NDC = v0.gl_Position.xyz() * iz.i[0];
    Vec3 p1_NDC = v1.gl_Position.xyz() * iz.i[1];
    Vec3 p2_NDC = v2.gl_Position.xyz() * iz.i[2];
    /* Step 3.2: Convert NDC space to window space */
    const double render_width = double(this->targets.color->w);
    const double render_height = double(this->targets.color->h);
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
    _bresenham_traversal(ip0.x, ip0.y, ip1.x, ip1.y, v0, v1, Vec2(iz.x, iz.y), uniforms);
    _bresenham_traversal(ip1.x, ip1.y, ip2.x, ip2.y, v1, v2, Vec2(iz.x, iz.y), uniforms);
    _bresenham_traversal(ip2.x, ip2.y, ip0.x, ip0.y, v2, v0, Vec2(iz.x, iz.y), uniforms);
  }
}

inline void
WireframePipeline::_inner_interpolate(
  int x, int y, double q,
  const Vertex_gl & v1, const Vertex_gl & v2,
  const Vec2 & iz)
{
  Vec2 w = Vec2(q, 1.0 - q);
  Vertex_gl v_lerp = v1 * w.i[0] + v2 * w.i[1];
  double z_real = 1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1]);
  v_lerp *= z_real;
  Fragment_gl fragment;
  assemble_fragment(v_lerp, fragment);
  double gl_FragDepth = (v_lerp.gl_Position.z / v_lerp.gl_Position.w + 1.0) * 0.5;
  gl_FragDepth *= 0.999;
  fragment.gl_FragCoord = Vec4(x, y, gl_FragDepth, 1.0 / v_lerp.gl_Position.w);
  write_render_targets(fragment.gl_FragCoord.xy(), Vec4(wppl.wire_color, 1.0), gl_FragDepth);
}

inline void
WireframePipeline::_bresenham_traversal(
  int x1, int y1, int x2, int y2,
  const Vertex_gl & v1, const Vertex_gl & v2,
  const Vec2 & iz, const Uniforms & uniforms)
{
  /* NOTE: internal drawing function, do not call it directly. */
  int dx, dy;
  int x, y;
  int epsilon = 0;
  int Dx = x2 - x1;
  int Dy = y1 - y2;
  Dx > 0 ? dx = +1 : dx = -1;
  Dy > 0 ? dy = -1 : dy = +1;
  Dx = abs(Dx), Dy = abs(Dy);
  if (Dx > Dy) {
    y = y1;
    for (x = x1; x != x2; x += dx) {
      /* process (x, y) here */
      double q = double(x2 - x) / double(Dx);
      _inner_interpolate(x, y, q, v1, v2, iz);
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
      _inner_interpolate(x, y, q, v1, v2, iz);
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
