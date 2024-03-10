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
    this->transform = transform;
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
    return this->transform;
  }

  /* ctor & dtor that we don't even care about much. */
  Model();
  ~Model(); /* currently i don't want to 
              declare it as "virtual". */
protected:
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  /* global transformation for the whole mesh, will be
   * applied before any other transformation during
   * rendering. */
  Mat4x4 transform;

private:
  /* Assimp model importer.
   * Note: if the importer is destoryed, the resources 
   * it holds will also be destroyed. */
  ::Assimp::Importer* importer;
  const aiScene* scene;

  void _register_vertex_weight(Vertex& v, uint32_t bone_index, double weight);

};

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
