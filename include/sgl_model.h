#pragma once

#include <string>
#include <vector>

#include "sgl_utils.h"
#include "zip.h" /* for loading zipped model files */
#include "sgl_pipeline.h"

/* Assimp: model import library */
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "assimp/material.h"

namespace sgl {

/**
Here we define our scene/model class to hold the model
data. Currently we only support loading static meshes.
NOTE: 
The loaded scene object can have multiple components.
We can apply different transformations to each of these 
components and achieve various interesting effects.
**/

class Mesh {
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
  /* draw mesh onto screen.
  @param ppl: the target pipeline object. The mesh will 
    actually be rendered onto the frame buffer that is 
    currently attached to the pipeline.
             mesh -> ppl -> pass -> texture
  */
  void draw(Pipeline& ppl, Pass& pass) const;
  /* Set mesh transformation */
  void set_transform(const Mat4x4& transform);

  /* ctor & dtor that we don't even care about much. */
  Mesh();
  ~Mesh(); /* currently i don't want to 
              declare it as "virtual". */
protected:
  /* here we only implements static mesh */
  struct MeshPart {
    /* a mesh is consists of multiple parts */
    std::vector<Vertex> vertices; 
    std::vector<int> indices;
    uint32_t mat_id; /* material id */
  };
  struct Material {
    /* each mesh part will only uses one material. */
    Texture diffuse_texture;
  };
  std::vector<MeshPart> parts;
  std::vector<Material> materials;
  /* global transformation for the whole mesh, will be
   * applied before any other transformation during
   * rendering. */
  Mat4x4 transform;

private:
  /* Assimp model importer.
   * Note: if the importer is destoryed, the resources 
   * it holds will also be destroyed. So we need to keep 
   * it alive when the model is being loaded. */
  ::Assimp::Importer* importer;
  /* Importer is not NULL if and only if the model is 
   * being loaded. After loading the model, the importer
   * itself is meaningless and will be destroyed. */
};

/* specialized VS and FS for mesh rendering. */
void
mesh_VS(const Vertex &vertex_in, const Uniforms &uniforms, 
    Vertex_gl& vertex_out);
void 
mesh_FS(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard);


};
