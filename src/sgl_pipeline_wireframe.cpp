#include "sgl_pipeline_wireframe.h"

namespace sgl {

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
  double gl_FragDepth = ((v_lerp.gl_Position.z / v_lerp.gl_Position.w) + 1.0) * 0.5;
  gl_FragDepth *= 0.9998; /* bring it up a little bit */
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


};

