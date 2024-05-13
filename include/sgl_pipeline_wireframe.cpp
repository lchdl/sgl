#include "sgl_pipeline_wireframe.h"

void sgl::WireframePipeline::vertex_processing(const VertexBuffer_t & vertex_buffer, const Uniforms & uniforms)
{
  for (uint32_t i_vert = 0; i_vert < vertex_buffer.size(); i_vert++) {
    Vertex_gl vertex_out;
    /* Map vertex from model local space to homogeneous clip space and stores to
    "gl_Position". */
    shaders.VS(uniforms, vertex_buffer[i_vert], vertex_out);
    ppl.Vertices.push_back(vertex_out);
  }
}
