#pragma once

#include <string>
#include <vector>

#include "zip.h" /* for loading zipped model files */
#include "sgl_utils.h"
#include "sgl_math.h"
#include "sgl_shader.h"

/* Assimp: model import library */
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "assimp/material.h"

namespace sgl {

struct Bone {
  struct VertexCtrl {
    uint32_t index; /* vertex index */
    double  weight; /* vertex weight */
  };
  /* bone name */
  std::string name; 
  /* vertices controlled by this bone */
  std::vector<VertexCtrl> vertices;
  /* transform vertex from local model space to bone space
     when the model is in bind pose (default T-pose). */
  Mat4x4 offset_matrix;
};
struct Mesh {
  /* A mesh is a unique part of a model that has only 
   * one material. A mesh can contain multiple meshes. */
  /* vertex buffer, used in rasterization */
  std::vector<Vertex> vertices; 
  /* index buffer, used in rasterization */
  std::vector<int32_t> indices;
  /* material id */
  uint32_t mat_id; 
  /* all the bones in this mesh */
  std::vector<Bone> bones;
  /* mapping bone name to its id. */
  std::map<std::string, uint32_t> bone_name_to_id; 
};
struct Material {
  /* each mesh part will only uses one material. */
  Texture diffuse_texture;
};

/**
Here we define our scene/model class to hold the model data.
**/
class Model {
  /* mesh represents a standalone object that can be 
   * rendered onto screen. A single draw call renders
   * a single mesh onto the frame buffer. Also from
   * the future plans I want to let each mesh object
   * stores their own animation(s). */
public:
  /* initialize mesh object from external/internal 
   * file formats. */
  bool load(const std::string& file);
  /* unload mesh and return allocated resources to OS. */
  void unload();
  /* Set mesh transformation */
  void set_transform(const Mat4x4& transform) {
    this->model_transform = transform;
  }
  /* get loaded mesh data */
  const std::vector<Mesh>& get_mesh_data() const {
    return this->meshes;
  }
  /* get model materials */
  const std::vector<Material>& get_materials() const {
    return this->materials;
  }
  /* get model transform, will be applied before any other transforms. */
  const Mat4x4 get_transform() const {
    return this->model_transform;
  }

  /* calculate bone final transformations and update
   * the result to uniform variables. */
  void update_bone_matrices_for_mesh(uint32_t i_mesh, Uniforms& uniforms);

  /* ctor & dtor that we don't even care about much. */
  Model();
  ~Model(); /* currently i don't want to 
              declare it as "virtual". */
protected:
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  /* global transformation for the whole model, will be
   * applied before any other transformation during
   * rendering. */
  Mat4x4 model_transform;

private:
  /* Assimp model importer.
   * Note: if the importer is destoryed, the resources 
   * it holds will also be destroyed. */
  ::Assimp::Importer* _importer;
  const aiScene* _scene;

  void _register_vertex_weight(Vertex& v, uint32_t bone_index, double weight);
  void _update_bone_matrices_from_node(
      const aiNode* node,
      const Mat4x4& parent_transform,
      const Mesh& mesh,
      Uniforms& uniforms);

};

inline Mat4x4 
convert_assimp_mat4x4(const aiMatrix4x4& m)
{
  return Mat4x4(
    m.a1, m.a2, m.a3, m.a4,
    m.b1, m.b2, m.b3, m.b4,
    m.c1, m.c2, m.c3, m.c4,
    m.d1, m.d2, m.d3, m.d4
  );
}

/* specialized VS and FS for mesh rendering. */
void
model_VS(const Vertex &vertex_in, const Uniforms &uniforms, 
    Vertex_gl& vertex_out);
void 
model_FS(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard);


class Animation {
  /*  */
};


};
