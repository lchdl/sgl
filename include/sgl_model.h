#pragma once

#include <string>
#include <vector>
#include <map>

#include "zip.h" /* for loading zipped model files */
#include "sgl_utils.h"
#include "sgl_math.h"
#include "sgl_enums.h"
#include "sgl_texture.h"
#include "sgl_shader.h"

/* 
Assimp: model import library
* Assimp github page:
  https://github.com/assimp/assimp
* Assimp model loading tutorial:
  https://www.youtube.com/watch?v=sP_kiODC25Q
*/
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/mesh.h"
#include "assimp/material.h"
#include "assimp/anim.h"

namespace sgl {

template <typename T> 
struct KeyFrame {
  double tick; 
  T value;
};
struct Animation {
  /* animation for a single bone */
  std::string name; /* name of the animation */
  std::vector<KeyFrame<Vec3>> scaling_key_frames;
  std::vector<KeyFrame<Vec3>> position_key_frames;
  std::vector<KeyFrame<Quat>> rotation_key_frames;
  double ticks_per_second; /* default = 25 */
};
struct Bone {
  /* bone name */
  std::string name; 
  /* transform vertex from local model space to bone space
     when the model is in bind pose (default T-pose). */
  Mat4x4 offset;
};
struct Node : public NonCopyable {
  /* node duplication requires deep copy operation (shallow copy disabled by design) */
  std::string          name; /* name of the node */
  Node*              parent; /* parent node name */
  std::vector<Node*> childs; /* child nodes */
  uint32_t        unique_id; /* unique node id */
  Mat4x4          transform; /* node transformation matrix */
  /* skeletal animations */
  std::vector<Animation> animations;
};
struct Mesh {
  typedef Vertex_pnt_nm_bone Vertex_t;
  /* A mesh is a unique part of a model that has only 
   * one material. A mesh can contain multiple meshes. */
  /* vertex buffer, used in rasterization */
  std::string name; /* name of the mesh */
  std::vector<Vertex_t> vertices;
  /* index buffer, used in rasterization */
  std::vector<int32_t> indices;
  /* material id */
  uint32_t mat_id; 
  /* all the bones in this mesh */
  std::vector<Bone> bones;
  /* mapping bone name to its index */
  std::map<std::string, uint32_t> bone_name_to_local_id; 
};
struct Material {
  /* each mesh part will only uses one material. */
  std::string diffuse_texture_file;
  Texture diffuse_texture;
};

class Model : public NonCopyable {
  /* The model represents a standalone object that 
   * can be rendered onto screen. A model can contain
   * one or multiple meshes. A single draw call only
   * renders a single mesh onto the frame buffer. */
public:
  /* A vertex can only be affected by no more than 4 bones.
   * NOTE: this value cannot be changed. */
  static const int MAX_BONES_INFLUENCE_PER_VERTEX = 4;
  /* A mesh model can only have less than 256 nodes. */
  static const int MAX_NODES_PER_MODEL = 256;

public:
  /* initialize mesh object from external/internal file formats. */
  bool load_zip(const std::string& zip_file, const std::string& model_fname);
  /* dump mesh information for debugging */
  void dump();
  /* unload mesh and return allocated resources to OS. */
  void unload();
  /* Set model global transformation matrix */
  void set_model_transform(const Mat4x4& transform) { this->model_transform = transform; }
  /* getters */
  const std::vector<Mesh>& get_meshes() const { return this->meshes; }
  const std::vector<Material>& get_materials() const { return this->materials; }
  const Mat4x4 get_model_transform() const { return this->model_transform; }

  /**
  update_skeletal_animation_for_mesh():
  * Calculate bone final transformations and update the result to 
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
                      v' = (T0*T1*T2*Q)*v = S*v,                 (1)
  where `S` is the collapsed transformation matrix. We call it `bone
  matrix` here to be convenient.
  
  Function update_bone_matrices_for_mesh() is used to calculate `S`
  for each bone, so that each vertex controlled by that bone can 
  quickly gain access to `S` when rendering. If a vertex is controlled
  by multiple bones B_i with weights w_i, then we need to do a simple
  linear interpolation for each bone, which means to calculate:
                      v' = sum(S_i*v) for each i,                (2)
  where S_i = w_i*S.

  During an animation sequence, we update each T_i in eq. (1) and 
  update the final vertex v' by calculating eq. (2).

  NOTE: If the model is in bind pose (default T-pose), then we will
        have: T0*T1*T2 = Q^-1, which means S is the identity matrix.
  **/
  void update_skeletal_animation_for_mesh(
    const Mesh& mesh,             /* the mesh being drawn */
    const std::string& anim_name, /* name of the animation being played */
    double play_time,             /* animation timeline (in sec.) */
    Mat4x4* bone_matrices         /* where results will be saved */
    /* NOTE: a single draw call only renders a single mesh onto screen,
    so if a model contains N meshes, it will need N draw calls to fully
    render the whole model, with i-th draw call renders the i-th mesh. */
  );
  void set_keyframe_interp_mode(const KeyFrameInterpType interp) {
    this->keyframe_interp_mode = interp;
  }

  /* ctor & dtor */

  Model();
  virtual ~Model();

protected:
  
  /*
  The following member variables must be initialized exclusively through this->load().
  No other initialization paths are permitted for these members.
  */

  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  /* global transformation for the whole model, will be
   * applied before any other transformation during
   * rendering. */
  Mat4x4 model_transform;
  /* animation name to animation id mapping */
  std::map<std::string, uint32_t> anim_name_to_unique_id;
  /* map a node name to a unique node id */
  std::map<std::string, uint32_t> node_name_to_unique_id;
  std::map<std::string, Node*> node_name_to_ptr;
  Node* root_node;
  /* key frame interpolation modes (nearest, linear, ...) */
  KeyFrameInterpType keyframe_interp_mode;

private:
  /* utility functions for loading the model */
  void _parse_and_copy_node(Node* node, aiNode* ai_node);
  void _delete_node(Node* node);

  /* animation related utility functions */
  void _register_vertex_weight(Vertex_pnt_nm_bone& v, uint32_t bone_ID, double weight);
  Node* _find_node_by_name(const std::string& node_name);
  Animation* _find_node_animation_by_name(Node& node, const std::string & anim_name);
  void _update_mesh_skeletal_animation_from_node(
    const Node* node,               /* current node being traversed */
    const Mat4x4& parent_transform, /* accumulated parent node transformation matrix */
    const Mesh& mesh,               /* mesh that contains all the bones */
    const uint32_t& anim_id,        /* id of the animation currently being played */
    double play_time,               /* elapsed time since the start of the animation (sec.) */
    Mat4x4* bone_matrices           /* uniform variables that will be written to */
  );
  Mat4x4 _interpolate_skeletal_animation(
    const Animation& anim, const double tick, const KeyFrameInterpType interp
  );

  /* utility functions for mesh debugging */
  void _dump_mesh(const Mesh& mesh);
  void _dump_material(const Material& material);
  void _dump_node(const Node* node, const uint32_t indent);
};

/*
Calculate tangent in local space of a triangle p0-p1-p2.
The texture coordinates of p0, p1, and p2 are t0, t1, and 
t2, respectively. Returns the calculated tangent vector
(normalized). This function is an auxiliary function for 
normal mapping.
* NOTE: p0, p1, and p2 are expressed in local model space.
* For the computation process of tangent vector, please
  visit: https://learnopengl.com/Advanced-Lighting/Normal-Mapping
  for more info.
*/
Vec3 calculate_tangent(
  const Vec3& p0, const Vec3& p1, const Vec3& p2,
  const Vec2& t0, const Vec2& t1, const Vec2& t2
);
void calculate_tangent_bitangent(
  const Vec3& p0, const Vec3& p1, const Vec3& p2,
  const Vec2& t0, const Vec2& t1, const Vec2& t2,
  Vec3& tangent, Vec3& bitangent
);

/*
Assimp data structure conversions.
*/
inline Mat4x4 convert_assimp_mat4x4(const aiMatrix4x4& m)
{
  return Mat4x4(
    m.a1, m.a2, m.a3, m.a4,
    m.b1, m.b2, m.b3, m.b4,
    m.c1, m.c2, m.c3, m.c4,
    m.d1, m.d2, m.d3, m.d4
  );
}
inline Vec3 convert_assimp_vec3(const aiVector3D& v) {
  return Vec3(v.x, v.y, v.z);
}
inline Quat convert_assimp_quat(const aiQuaternion& q) {
  return Quat(q.w, q.x, q.y, q.z);
}

}; /* namespace sgl */
