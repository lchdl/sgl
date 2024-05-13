#pragma once
#include "sgl_pipeline.h"

namespace sgl {

class WireframePipeline : public Pipeline {
public:
  void set_wireframe_color(const Vec3& color) { wppl.wire_color = color; }
  /* wireframe pipeline only support single-threaded rendering but default draw()
  implementation is multi-threaded, so we need to rewrite it. */
  virtual void draw(
    const std::vector<Vertex>& vertices,
    const std::vector<int32_t>& indices,
    const Uniforms& uniforms);
  virtual void draw(
    const int32_t& vbo,
    const int32_t& ibo,
    const Uniforms& uniforms
  );

public:
  WireframePipeline() {};
  virtual ~WireframePipeline() {};

protected:
  void fragment_processing(const Uniforms &uniforms);

protected:
  void _inner_interpolate(
    int x, int y, double q, 
    const Vertex_gl& v1, const Vertex_gl& v2, 
    const Vec2& iz);
  void _bresenham_traversal(
    int x1, int y1, int x2, int y2, 
    const Vertex_gl& v1, const Vertex_gl& v2,
    const Vec2& iz, const Uniforms& uniforms);

protected:
  struct {
    Vec3 wire_color;
  } wppl;
};

}; /* namespace sgl */
