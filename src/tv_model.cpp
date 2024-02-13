#include "tv_model.h"
#include "tv_model_impl.h"

tv_mesh *tv_mesh_create() {
  tv_mesh *mesh = new tv_mesh();
  return mesh;
}
void tv_mesh_destroy(tv_mesh *mesh) {
  if (mesh) {
    delete mesh;
  }
}
int tv_mesh_add_vertex(tv_mesh *mesh, const tv_vec3 &p, const tv_vec3 &n,
                       const tv_vec2 &t) {
  tv_vertex vert;
  vert.p = p;
  vert.n = tv_normalize(n);
  vert.t = t;
  mesh->v_buf.push_back(vert);
  return mesh->v_buf.size() - 1;
}

int tv_mesh_add_triangle(tv_mesh *mesh, const int &i1, const int &i2,
                          const int &i3)
{
  mesh->i_buf.push_back(i1);
  mesh->i_buf.push_back(i2);
  mesh->i_buf.push_back(i3);
  return mesh->i_buf.size()/3-1;
}
