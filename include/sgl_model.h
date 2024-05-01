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
struct Animation {
  /*  */
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

class Model {
  /* mesh represents a standalone object that can be 
   * rendered onto screen. A single draw call renders
   * a single mesh onto the frame buffer. Also from
   * the future plans I want to let each mesh object
   * stores their own animation(s). */
public:
  /* initialize mesh object from external/internal file formats. */
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

  /**
	update_bone_matrices_for_mesh():
	Calculate bone final transformations and update the result to 
	uniform variables.

	              * THE PRINCIPLE BEHIND BONE ANIMATION *              
	-------------------------------------------------------------------
	If a model has the following bone hierarchy (B0->B1->B2),
	and a vertex `v` is affected by bone B2 (shown as follows):
	
	B0
	+--B1
		+--B2
				+--v
	
	There are two kinds of matrices we need to know:
	1. The bone's OFFSET MATRIX
	2. The bone's TRANSFORMATION MATRIX
	
	The OFFSET MATRIX is used to transform a vertex from local model
	space directly to bone's local space, while the TRANSFORMATION
	MATRIX is used to transform a vertex from bone's local space to
	parent bone's local space. Here we assume the TRANSFORMATION 
	MATRIX for bone B0, B1, and B2 are T0, T1 and T2, respectively. 
	B2's OFFSET MATRIX is Q.
	Q represents the transformation from model space to B2's local space.
	T2 represents the transformation from B2's local space to B1's local space, 
	T1 represents the transformation from B1's local space to B0's local space.
	T0 represents the transformation from B0's local space to model space.

	To calculate the real position for vertex v, first we need
	to transform vertex v from local model space to B2's local space,
	which can be calculated from: Q*v.
	Then we can transform from B2's local space back to model space by
	calculating:
	                       v' = (T0*T1*T2*Q)*v = S*v,              (1)
	where `S` is the collapsed transformation matrix. We call it `bone
	matrix` here to be convenient.
	
	Function update_bone_matrices_for_mesh() is used to calculate `S`
	for each bone, so that each vertex controlled by that bone can 
	quickly gain access to `S` when rendering. If a vertex is controlled
	by multiple bones B_i with weights w_i, then we need to do a simple
	linear interpolation for each bone, which means to calculate:
	                        v' = sum(S_i*v) for i,                 (2)
	where S_i = w_i*S.

	During an animation sequence, we update each T_i in eq. (1) and 
	update the final vertex v' by calculating eq. (2).

	NOTE: If the model is in bind pose (default T-pose), then we will
	      have: T0*T1*T2 = Q^-1, which means S is the identity matrix.
	**/
  void update_bone_matrices_for_mesh(uint32_t i_mesh, Uniforms& uniforms);

  /* ctor & dtor that we don't even care about much. */
  Model();
  ~Model(); /* currently i don't want to declare it as "virtual". */
protected:
  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  std::vector<Animation> animations;
  /* global transformation for the whole model, will be
   * applied before any other transformation during
   * rendering. */
  Mat4x4 model_transform;
  /* the model is not gauranteed to be placed at the world's origin,
   * we need to place the model to the origin by multiplying the
   * global inverse transform matrix that is calculated when loading
   * the model data. */
  Mat4x4 global_inverse_transform;

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
void model_VS(
  const Vertex &vertex_in, 
  const Uniforms &uniforms, 
  Vertex_gl& vertex_out);
void model_FS(
  const Fragment_gl &fragment_in, 
  const Uniforms &uniforms,
  Vec4 &color_out, bool& discard);

};
