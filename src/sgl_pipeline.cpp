#include "sgl_pipeline.h"

#include <malloc.h>
#include <omp.h>

namespace sgl {

Mat4x4 
Pass::get_view_matrix() const {
  Vec3 front = normalize(eye.position - eye.look_at);
  Vec3 left = normalize(cross(eye.up_dir, front));
  Vec3 up = normalize(cross(front, left));
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
Pass::get_projection_matrix() const {
  Mat4x4 projection_matrix;
  if (eye.perspective.enabled) {
    double aspect_ratio = double(color_texture->w) / double(color_texture->h);
    double inv_aspect = double(1.0) / aspect_ratio;
    double near = eye.perspective.near;
    double far = eye.perspective.far;
    double field_of_view = eye.perspective.field_of_view;
    double left = -tan(field_of_view / double(2.0)) * near;
    double right = -left;
    double top = inv_aspect * right;
    double bottom = -top;
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

Pass::Pass()
{
  color_texture = NULL;
  depth_texture = NULL;

  eye.look_at = Vec3(0, 0, 0);
  eye.position = Vec3(10, 10, 10);
  eye.up_dir = Vec3(0, 1, 0);

  eye.perspective.enabled = true;
  eye.perspective.near = 0.1;
  eye.perspective.far = 100.0;
  eye.perspective.field_of_view = PI / 4.0;

  eye.orthographic.enabled = false;
  eye.orthographic.width = 256.0;
  eye.orthographic.height = 256.0;
  eye.orthographic.depth = 256.0;

  VS = NULL;
  FS = NULL;
}

Pass::~Pass() {
}

ModelPass::ModelPass() {
  model = NULL;
}
ModelPass::~ModelPass() {
}

void
ModelPass::run(Pipeline& ppl) {
  if (model==NULL) return;

  ppl.set_shaders(model_VS, model_FS);
  ppl.set_render_targets(this->color_texture, this->depth_texture);
  ppl.clear_textures(this->color_texture, this->depth_texture, Vec4(0.5,0.5,0.5,1.0));

  uniforms.model = model->get_transform();
  uniforms.view = this->get_view_matrix();
  uniforms.projection = this->get_projection_matrix();

  /* Rendering all the mesh parts in model */
  const std::vector<Mesh>& mesh_data = model->get_mesh_data();
  const std::vector<Material>& materials = model->get_materials();

  for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
    const std::vector<Vertex>& vertices = mesh_data[i_mesh].vertices;
    const std::vector<int32_t>& indices = mesh_data[i_mesh].indices;
    const int32_t mat_id = mesh_data[i_mesh].mat_id;
    /* Setting up mesh materials. */
    /* diffuse texture */
    uniforms.in_textures[0] = &materials[mat_id].diffuse_texture;
    /* calculate bone tranformation matrices and update to uniforms */
    this->model->update_bone_matrices_for_mesh(i_mesh, uniforms);
    /* Launch the pipeline to render the triangles */
    ppl.draw(vertices, indices, uniforms);
  }
}


Pipeline::Pipeline() {
  target.color = NULL;
  target.depth = NULL;
	shaders.VS = NULL;
	shaders.FS = NULL;
  hwspec.num_threads = max(get_cpu_cores(), 1);
}
Pipeline::Pipeline(VS_func_t VS, FS_func_t FS)
{
	target.color = NULL;
	target.depth = NULL;
	shaders.VS = VS;
	shaders.FS = FS;
	hwspec.num_threads = max(get_cpu_cores(), 1);
}
Pipeline::~Pipeline() {}

void Pipeline::draw(
  const std::vector<Vertex>& vertices,
  const std::vector<int32_t>& indices,
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

  /* Step 3: Rasterization & fragment processing */
  fragment_processing(uniforms);
}

void
Pipeline::vertex_processing(const std::vector<Vertex> &vertex_buffer,
                            const Uniforms &uniforms) {
  for (uint32_t i_vert = 0; i_vert < vertex_buffer.size(); i_vert++) {
    Vertex_gl vertex_out;
    /* Map vertex from model local space to homogeneous clip space and stores to
    "gl_Position". */
    shaders.VS(vertex_buffer[i_vert], uniforms, vertex_out);
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
    /** Step 2.2: Face culling.
    @note: After vertex shader, all vertices are in homogeneous space,
    and since all vertices were transformed by view matrix, now the eye vector
    is fixed to (0,0,-1), and dot product between eye vector and triangle normal
    simply becomes -n.z. If -n.z > 0.0, then ignore this face.
    **/
    Vec3 p0 = tri_gl.v[0].gl_Position.xyz();
    Vec3 p1 = tri_gl.v[1].gl_Position.xyz();
    Vec3 p2 = tri_gl.v[2].gl_Position.xyz();

    Vec3 facing = cross(p1 - p0, p2 - p0);
    if (facing.z < 0.0)
      continue;
    /** Step 2.3: Clipping.
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
    Vec3 p0_NDC = v0.gl_Position.xyz() / v0.gl_Position.w;
    Vec3 p1_NDC = v1.gl_Position.xyz() / v1.gl_Position.w;
    Vec3 p2_NDC = v2.gl_Position.xyz() / v2.gl_Position.w;
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
    const double render_width = double(this->target.color->w);
    const double render_height = double(this->target.color->h);
    const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
    const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w,
                         1.0 / v2.gl_Position.w);
    const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
    const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
    const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
    double area = edge(p0, p1, p2);
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
        if (w.i[0] < 0 || w.i[1] < 0 || w.i[2] < 0)
          continue;
        w /= area;
        Vertex_gl v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
        double z_real =
            1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
        v_lerp *= z_real;
        /**
        @note: v_lerp is the interpolated vertex in homogeneous clip space
        in order to assemble the correct gl_FragCoord, we need to manually
        convert it into NDC space again.
        **/
        /* Step 3.4: Assemble fragment and render pixel. */
        Fragment_gl fragment;
        assemble_fragment(v_lerp, fragment);
        fragment.gl_FragCoord =
            Vec4(p.x, p.y, (1.0 + v_lerp.gl_Position.z) / 2.0,
                 1.0 / v_lerp.gl_Position.w);
        Vec4 color_out;
				bool discard = false;
				shaders.FS(fragment, uniforms, color_out, discard);
        /* Step 3.5: Fragment processing */
				if (!discard) {
					write_textures(fragment.gl_FragCoord.xy(), color_out,
						fragment.gl_FragCoord.z);
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
      Vec3 p0_NDC = v0.gl_Position.xyz() / v0.gl_Position.w;
      Vec3 p1_NDC = v1.gl_Position.xyz() / v1.gl_Position.w;
      Vec3 p2_NDC = v2.gl_Position.xyz() / v2.gl_Position.w;
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
      const double render_width = double(this->target.color->w);
      const double render_height = double(this->target.color->h);
      const Vec3 scale_factor = Vec3(render_width, render_height, 1.0);
      const Vec3 iz = Vec3(1.0 / v0.gl_Position.w, 1.0 / v1.gl_Position.w,
                           1.0 / v2.gl_Position.w);
      const Vec4 p0 = Vec4(0.5 * (p0_NDC + 1.0) * scale_factor, iz.i[0]);
      const Vec4 p1 = Vec4(0.5 * (p1_NDC + 1.0) * scale_factor, iz.i[1]);
      const Vec4 p2 = Vec4(0.5 * (p2_NDC + 1.0) * scale_factor, iz.i[2]);
      double area = edge(p0, p1, p2);
      /** @note: p0, p1, p2 are actually gl_FragCoord. **/
      /* Step 3.3: Rasterization. */
      Vec4 rect = get_minimum_rect(p0, p1, p2);
      /* precomupte: divide by real z */
      v0 *= iz.i[0];
      v1 *= iz.i[1];
      v2 *= iz.i[2];
      Vec4 p;
      int y_base = num_threads * int(int(rect.i[1]) / num_threads);
      for (p.y = double(y_base) + 0.5 + double(thread_id); p.y < rect.i[3];
           p.y += double(num_threads)) {
        for (p.x = floor(rect.i[0]) + 0.5; p.x < rect.i[2]; p.x += 1.0) {
          /**
          @note: here the winding order is important,
          and w_i are calculated in window space
          **/
          Vec3 w = Vec3(edge(p1, p2, p), edge(p2, p0, p), edge(p0, p1, p));
          /* discard pixel if it is outside the triangle area */
          if (w.i[0] < 0 || w.i[1] < 0 || w.i[2] < 0)
            continue;
          w /= area;
          Vertex_gl v_lerp = v0 * w.i[0] + v1 * w.i[1] + v2 * w.i[2];
          double z_real =
              1.0 / (iz.i[0] * w.i[0] + iz.i[1] * w.i[1] + iz.i[2] * w.i[2]);
          v_lerp *= z_real;
          /**
          @note: v_lerp is the interpolated vertex in homogeneous clip space
          in order to assemble the correct gl_FragCoord, we need to manually
          convert it into NDC space again.
          **/
          /* Step 3.4: Assemble fragment and render pixel. */
          Fragment_gl fragment;
          assemble_fragment(v_lerp, fragment);
          fragment.gl_FragCoord =
              Vec4(p.x, p.y, (1.0 + v_lerp.gl_Position.z) / 2.0,
                   1.0 / v_lerp.gl_Position.w);
          Vec4 color_out;
					bool discard = false;
          shaders.FS(fragment, uniforms, color_out, discard);
          /* Step 3.5: Fragment processing */
					if (!discard) {
						write_textures(fragment.gl_FragCoord.xy(), color_out,
							fragment.gl_FragCoord.z);
					}
        }
      }
    }
  }
}

void
Pipeline::write_textures(const Vec2 &p, const Vec4 &color, const double &z) {
  int w = this->target.color->w;
  int h = this->target.color->h;
  int ix = int(p.x);
  int iy = h - 1 - int(p.y);
  if (ix < 0 || ix >= w || iy < 0 || iy >= h)
    return;
  uint8_t *pixels = (uint8_t *) this->target.color->pixels;
  int pixel_id = iy * w + ix;
  /* depth test */
  double *depths = (double *) this->target.depth->pixels;
  double z_new = min(max(z, 0.0), 1.0);
  double z_orig = depths[pixel_id];
  if (z_orig < z_new)
    return;
  depths[pixel_id] = z_new;
  uint8_t R, G, B, A;
  convert_Vec4_to_RGBA8(color, R, G, B, A);
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
  for (uint32_t clip_axis = 0; clip_axis < 3; clip_axis++) {
    for (uint32_t i_clip = 0; i_clip < 2; i_clip++) {
      int clip_sign = clip_signs[i_clip];
      for (uint32_t i_tri = 0; i_tri < Qcur->size(); i_tri++) {
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
  } else if (p1_sign > 0 && p2_sign > 0 && p3_sign > 0) {
    /* triangle is completely inside clipping volume, we don't need to do any
     * clipping operations */
    n_tri = 1;
    q1 = v1, q2 = v2, q3 = v3;
  } else {
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
    Assume we have two vertices A and B, segment A-B will be clipped by a
    plane, assume the intersection is C, such that C = (1-t)A + tB. Then we
    have:
                              C_w = (1-t)*A_w + t*B_w.
    For the near plane (z-axis) clipping, we have:
                                     C_z = -C_w.
    Since C_z =(1-t)*A_z + t*B_z, then we also have:
                     -(1-t)*A_w - t*B_w = (1-t)*A_z + t*B_z.
    We can solve scalar t:
                       t = (A_z+A_w) / ((A_z+A_w)-(B_z+B_w)).
    Similarily, we can solve scalar t for far plane clipping:
                       t = (A_z-A_w) / ((A_z-A_w)-(B_z-B_w)).
    Clipping with other axes is also the same. Just replace A_z to A_x, A_y; B_z
    to B_x or B_y.
    @note: The scalar t can also be used to interpolate all the associated
    vertex attributes for C. The linear interpolation is perfectly sufficient
    even in perspective distorted cases, because we are before the perspective
    divide here, were the whole perspective transformation is perfectly affine
    wrt. the 4D space we work in.
    **/
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
Pipeline::clear_textures(
  Texture* color, 
  Texture* depth,
  const Vec4 &clear_color)
{ 
  uint8_t R, G, B, A;
  convert_Vec4_to_RGBA8(clear_color, R, G, B, A);
  if (color != NULL) {
    int n_pixels = color->w * color->h;
    uint8_t *pixels = (uint8_t *) color->pixels;
    for (int i = 0; i < n_pixels; i++) {
      pixels[i * 4 + 0] = R;
      pixels[i * 4 + 1] = G;
      pixels[i * 4 + 2] = B;
      pixels[i * 4 + 3] = A;
    }
  }
  if (depth != NULL) {
    int n_pixels = depth->w * depth->h;
    double *pixels = (double *) depth->pixels;
    for (int i = 0; i < n_pixels; i++) {
      pixels[i] = 100.0;
    }
  }
}

};   // namespace sgl
