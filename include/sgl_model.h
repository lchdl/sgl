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

struct MeshData {
  /* a mesh is consists of multiple parts */
  std::vector<Vertex> vertices; 
  std::vector<int32_t> indices;
  uint32_t mat_id; /* material id */
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
  const std::vector<MeshData>& get_mesh_data() const {
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
  std::vector<MeshData> meshes;
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
