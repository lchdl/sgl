#include "sgl_model.h"
#include "sgl_math.h"
#include "sgl_utils.h"
#include <string>
#include <vector>

namespace sgl {

Model::Model() {
  this->root_node = NULL;
  this->model_transform = Mat4x4::identity();
  this->keyframe_interp_mode = KeyFrameInterpType::KeyFrameInterpType_Linear;
}
Model::~Model() {
  this->unload();
}

Model::Model(const Model& that) {
  this->root_node = NULL;
  this->model_transform = Mat4x4::identity();
  this->keyframe_interp_mode = KeyFrameInterpType::KeyFrameInterpType_Linear;

  if (that.load_info.load_method == "load_zip") {
    this->load_zip(that.load_info.zip_file, that.load_info.model_fname);
  }
  /* TODO: add other copy methods if load_method is new */

  this->set_model_transform(that.model_transform);
  this->set_keyframe_interp_mode(that.keyframe_interp_mode);
}

Model& Model::operator=(const Model& that) {
  if (this == &that)
    return (*this);

  this->unload();

  this->root_node = NULL;
  this->model_transform = Mat4x4::identity();
  this->keyframe_interp_mode = KeyFrameInterpType::KeyFrameInterpType_Linear;

  if (that.load_info.load_method == "load_zip") {
    this->load_zip(that.load_info.zip_file, that.load_info.model_fname);
  }

  this->set_model_transform(that.model_transform);
  this->set_keyframe_interp_mode(that.keyframe_interp_mode);

  return (*this);
}

void Model::unload() {
  this->load_info.load_method = "";
  this->load_info.zip_file = "";
  this->load_info.model_fname = "";
  this->meshes.clear();
  this->materials.clear();
  this->_delete_node(root_node);
  this->root_node = NULL;
  this->model_transform = Mat4x4::identity();
  this->anim_name_to_unique_id.clear();
  this->node_name_to_unique_id.clear();
  this->node_name_to_ptr.clear();
  this->keyframe_interp_mode = KeyFrameInterpType::KeyFrameInterpType_Linear;
}

bool Model::load_zip(const std::string& zip_file, const std::string& model_fname) {
  
  /* clear trash data from previous load */
  this->unload(); 
  
  this->load_info.load_method = "load_zip";
  this->load_info.zip_file = zip_file;
  this->load_info.model_fname = model_fname;

  /* Assimp model importer.
   * Note: if the importer is destoryed, the resources
   * it holds will also be destroyed. */
  ::Assimp::Importer* _importer = new ::Assimp::Importer();
  const aiScene* _scene;
  
  /* if the model is packed as a *.zip file, unpack it first */
  std::string temp_folder = "";
  std::string model_file = "";


  if (endswith(zip_file, ".zip")) {
    temp_folder = mktdir(gd(zip_file));
    int zipret = zip_extract(zip_file.c_str(), temp_folder.c_str(), NULL, NULL);
    if (zipret < 0) {
      printf("Assimp import error: cannot unzip file \"%s\".", zip_file.c_str());
      rm(temp_folder);
      return false;
    }
    std::vector<std::string> files = ls(temp_folder);
    for (auto& file : files) {
      size_t dpos0 = file.find_last_of("\\");
      size_t dpos1 = file.find_last_of(".");
      std::string file_no_ext = file.substr(dpos0+1, dpos1-dpos0-1);
      std::string file_ext = file.substr(dpos1 + 1);
      if (file_no_ext + '.' + file_ext == model_fname) {
        model_file = temp_folder + '\\' + model_fname;
        break;
      }
    }
    if (model_file == "") {
      printf("Model import error: cannot find model \"%s\" in zip file \"%s\".\n", model_fname.c_str(), zip_file.c_str());
      rm(temp_folder);
      return false;
    }
  }
  else {
    printf("File name must ends with \".zip\".\n");
    return false;
  }
  
  /* then import the file using assimp */
  uint32_t load_flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;
  _scene = _importer->ReadFile(model_file.c_str(), load_flags);
  if (!_scene || !_scene->mRootNode || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
    printf("Assimp importer.ReadFile() error when loading file \"%s\": \"%s\".\n",
      model_file.c_str(), _importer->GetErrorString());
    if (temp_folder != "")
      rm(temp_folder);
    return false;
  }

  /* In Assimp, a scene consists of multiple meshes, each mesh can 
   * only have one material. If a mesh uses multiple materials for 
   * its surface, it will be split up to multiple sub-meshes so 
   * that each sub-mesh only uses one material. Here we load all 
   * sub-meshes in a scene, and each sub-mesh in Assimp will be 
   * considered as a `mesh part` in here. */

  /* parse node hierarchy */
  root_node = new Node();
  root_node->parent = NULL;
  _parse_and_copy_node(root_node, _scene->mRootNode);

  /* parse meshes */
  uint32_t n_meshes = _scene->mNumMeshes;
  this->meshes.resize(n_meshes);
  const aiVector3D zvec = aiVector3D(0.0, 0.0, 0.0);
  for(uint32_t i_mesh = 0; i_mesh < n_meshes; i_mesh++) {
    const aiMesh* mesh = _scene->mMeshes[i_mesh];
    const uint32_t n_vert = mesh->mNumVertices;
    this->meshes[i_mesh].name = mesh->mName.data;
    /* load vertex (positions, normals, and texture coordinates) */
    for (uint32_t i_vert = 0; i_vert < n_vert; i_vert++) {
      const aiVector3D* position  = &mesh->mVertices[i_vert];
      const aiVector3D* normal    = &mesh->mNormals[i_vert];
      const aiVector3D* texcoord  = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i_vert] : &zvec;
      const aiVector3D* tangent   = &mesh->mTangents[i_vert];
      const aiVector3D* bitangent = &mesh->mBitangents[i_vert];
      Vertex_pnt_nm_bone v;
      v.position  = Vec3(double(position->x),  double(position->y),  double(position->z));
      v.normal    = Vec3(double(normal->x),    double(normal->y),    double(normal->z));
      v.texcoord  = Vec2(double(texcoord->x),  double(texcoord->y));
      v.tangent   = Vec3(double(tangent->x),   double(tangent->y),   double(tangent->z));
      v.bitangent = Vec3(double(bitangent->x), double(bitangent->y), double(bitangent->z));
      v.bone_IDs     = IVec4(-1,-1,-1,-1);
      v.bone_weights = Vec4(0.0, 0.0, 0.0, 0.0);
      v.tangent   = normalize(v.tangent);
      v.bitangent = normalize(v.bitangent);
      v.normal = normalize(v.normal);
      //print(cross(v.tangent, v.bitangent));
      //print(v.normal);
      this->meshes[i_mesh].vertices.push_back(v);
    }
    /* load triangle face indices */
    for (uint32_t i_face = 0; i_face < mesh->mNumFaces; i_face++) {
      const aiFace& face = mesh->mFaces[i_face];
      this->meshes[i_mesh].indices.push_back(face.mIndices[0]);
      this->meshes[i_mesh].indices.push_back(face.mIndices[1]);
      this->meshes[i_mesh].indices.push_back(face.mIndices[2]);
    }
    this->meshes[i_mesh].mat_id = mesh->mMaterialIndex;
    
    /* bones and animation support:
     * For each bone (aiBone) object, "mOffsetMatrix" stores the
     * transformation from local model space directly to bone space 
     * in bind pose (default T-pose). */

    /* load all the bones */
    for (uint32_t i_bone = 0; i_bone < mesh->mNumBones; i_bone++) {
      Bone bone;
      bone.name = mesh->mBones[i_bone]->mName.data;
      bone.offset = convert_assimp_mat4x4(mesh->mBones[i_bone]->mOffsetMatrix);
      /* number of affected vertices by this bone */
      uint32_t n_bone_verts = mesh->mBones[i_bone]->mNumWeights;
      for (uint32_t i_vert = 0; i_vert < n_bone_verts; i_vert++) {
        /* read and save all info about the affected vertices by this bone */
        aiVertexWeight vw = mesh->mBones[i_bone]->mWeights[i_vert];
        /* write bone info into affected vertex (let the vertex know
         * there is a bone that influences itself). */
        Vertex_pnt_nm_bone& affected_vert = this->meshes[i_mesh].vertices[vw.mVertexId];
        uint32_t node_unique_id = this->node_name_to_unique_id[bone.name];
        _register_vertex_weight(affected_vert, node_unique_id, vw.mWeight);
      }
      /* register bone */
      std::vector<Bone>& bones_list = this->meshes[i_mesh].bones;
      bones_list.push_back(bone);
      this->meshes[i_mesh].bone_name_to_local_id.insert_or_assign(bone.name, (uint32_t)bones_list.size() - 1);
    }
  }

  /* parse model animation(s) (if exists) */
  uint32_t n_anims = _scene->mNumAnimations;
  for (uint32_t i_anim = 0; i_anim < n_anims; i_anim++) {
    const aiAnimation* anim = _scene->mAnimations[i_anim];
    double ticks_per_second = anim->mTicksPerSecond;
    if (ticks_per_second < 1.0) ticks_per_second = 25.0;
    uint32_t n_ctrl_nodes = anim->mNumChannels; /* number of bones this animation controls */
    std::string anim_name = anim->mName.data;
    /* register this animation */
    std::map<std::string, uint32_t>::const_iterator item = anim_name_to_unique_id.find(anim_name);
    if (item != anim_name_to_unique_id.end()) {
      printf("Found duplicated animation \"%s\".\n", anim_name.c_str());
    }
    anim_name_to_unique_id.insert_or_assign(anim_name, (uint32_t)anim_name_to_unique_id.size());
    /* loop for each bone this animation controls, fill in node->animations */
    for (uint32_t i_channel = 0; i_channel < n_ctrl_nodes; i_channel++) {
      const aiNodeAnim* node_anim = anim->mChannels[i_channel];
      std::string node_name = node_anim->mNodeName.data;
      Node* node = _find_node_by_name(node_name);
      /* read bone animation key frames */
      Animation* dst_anim = _find_node_animation_by_name(*node, anim_name);
      if (dst_anim == NULL) {
        /* create new animation if not exist */
        Animation new_anim;
        new_anim.name = anim_name;
        new_anim.ticks_per_second = ticks_per_second;
        node->animations.push_back(new_anim);
        dst_anim = &(node->animations[node->animations.size() - 1]);
      }
      /* read key frames and store to current animation */
      for (uint32_t i_key = 0; i_key < node_anim->mNumScalingKeys; i_key++) {
        KeyFrame<Vec3> key_frame;
        key_frame.tick = node_anim->mScalingKeys[i_key].mTime;
        key_frame.value = convert_assimp_vec3(node_anim->mScalingKeys[i_key].mValue);
        dst_anim->scaling_key_frames.push_back(key_frame);
      }
      for (uint32_t i_key = 0; i_key < node_anim->mNumPositionKeys; i_key++) {
        KeyFrame<Vec3> key_frame;
        key_frame.tick = node_anim->mPositionKeys[i_key].mTime;
        key_frame.value = convert_assimp_vec3(node_anim->mPositionKeys[i_key].mValue);
        dst_anim->position_key_frames.push_back(key_frame);
      }
      for (uint32_t i_key = 0; i_key < node_anim->mNumRotationKeys; i_key++) {
        KeyFrame<Quat> key_frame;
        key_frame.tick = node_anim->mRotationKeys[i_key].mTime;
        key_frame.value = convert_assimp_quat(node_anim->mRotationKeys[i_key].mValue);
        dst_anim->rotation_key_frames.push_back(key_frame);
      }
    }
  }

  /* load materials */
  uint32_t n_materials = _scene->mNumMaterials;
  this->materials.resize(n_materials);
  for (uint32_t i_mat = 0; i_mat < n_materials; i_mat++) {
    /* load materials */
    const aiMaterial* material = _scene->mMaterials[i_mat];
    /* load diffuse texture (if exists) */
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0){
      aiString _tp;
      if (material->GetTexture(aiTextureType_DIFFUSE, 0, &_tp, 
            NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
        std::string tp = _tp.data;
#if defined (LINUX)
        replace_all(tp, "\\", "/");
        /* remove duplicated '/' characters in file path. "///" -> "/" */
        while (tp.find("//") != std::string::npos)
          replace_all(tp, "//", "/");
#endif
        std::string tex_full_path = join(gd(model_file), tp);
        /* create texture object and append to mesh texture library */
        this->materials[i_mat].diffuse_texture = load_texture(tex_full_path, PixelFormat_BGRA8888, TextureSampling_Nearest, true);
        this->materials[i_mat].diffuse_texture_file = tex_full_path;
        if (this->materials[i_mat].diffuse_texture.get_pixel_data() == NULL) {
          printf("Texture loading error: cannot load texture \"%s\". "
              "File not exist or have no access.\n", tex_full_path.c_str());
        }
      }
    }
    /* TODO: load other types of textures (if exists) */
  }
  
  /* parse ended, now cleaning up... */
  /* if model is loaded from an unpacked zip file, remove the temporary dir. */
  if (temp_folder != "")
    rm(temp_folder);
  delete _importer;
  
  return true;
}

void Model::dump()
{
  printf("Model dump:\n");
  printf("  Total number of mesh(es): %zd\n", this->meshes.size());
  for (uint32_t i_mesh = 0; i_mesh < this->meshes.size(); i_mesh++) {
    printf("  Mesh [%d]: \"%s\"\n", i_mesh, this->meshes[i_mesh].name.c_str());
    this->_dump_mesh(this->meshes[i_mesh]);
  }
  printf("  Nodes:\n");
  this->_dump_node(root_node, 2);
}

void Model::_parse_and_copy_node(Node* node, aiNode* ai_node)
{
  std::string node_name = ai_node->mName.data;
  node->name = node_name;
  node->unique_id = (uint32_t)node_name_to_unique_id.size();
  if (node->unique_id >= MAX_NODES_PER_MODEL) {
    printf("[*] Warning: maximum number of nodes per mesh (%d) "
      "exceeded when registering node \"%s\".", 
      MAX_NODES_PER_MODEL, node->name.c_str());
  }
  node->transform = convert_assimp_mat4x4(ai_node->mTransformation);
  node_name_to_unique_id.insert_or_assign(node_name, (uint32_t)node_name_to_unique_id.size());
  node_name_to_ptr.insert_or_assign(node_name, node);
  for (uint32_t i_node = 0; i_node < ai_node->mNumChildren; i_node++) {
    Node* child_node = new Node();
    child_node->parent = node;
    node->childs.push_back(child_node);
    _parse_and_copy_node(child_node, ai_node->mChildren[i_node]);
  }
}

void Model::_delete_node(Node * node)
{
  if (node == NULL) return;
  for (uint32_t i = 0; i < node->childs.size(); i++)
    this->_delete_node(node->childs[i]);
  delete node;
}

void
Model::_register_vertex_weight(
  Vertex_pnt_nm_bone& v, uint32_t bone_ID, double weight) 
{
  /* insert & sort vertex weights in descent order,
   * in this way, only top-k bones will be kept for
   * each vertex. */
  bool registered = false;
  for (uint32_t i=0; i<MAX_BONES_INFLUENCE_PER_VERTEX; i++) {
    if (weight > v.bone_weights[i]) {
      /* insert bone to this slot */
      registered = true;
      if (v.bone_IDs[MAX_BONES_INFLUENCE_PER_VERTEX - 1] >= 0) {
        printf("[*] Warning: A weight will be ignored because there are "
          "more than 4 bones affecting this vertex.\n");
      }
      /* shift right */
      for (uint32_t j=MAX_BONES_INFLUENCE_PER_VERTEX-1; j>i; j--) {
        v.bone_weights[j] = v.bone_weights[j-1];
        v.bone_IDs[j] = v.bone_IDs[j-1];
      }
      /* insert */
      v.bone_weights[i] = weight;
      v.bone_IDs[i] = bone_ID;
      break;
    }
  }
  if (registered == false) {
    /* All 4 slots have been occupied, we print a warning to let user
     * know and then continue. */
    printf("[*] Warning: Cannot register vertex weight (bone_ID=%d, "
           "weight=%.4lf), all 4 slots have been occupied. Ignored.\n", 
           bone_ID, weight);
  }
}
void
Model::_update_mesh_skeletal_animation_from_node(
  const Node* node, const Mat4x4& parent_transform, const Mesh& mesh,
  const uint32_t& anim_id, double play_time, Mat4x4* bone_matrices)
{
  /* First we retrieve some info about this node. In Assimp, if a node 
  is actually a bone, then the node name will be set to be the same as 
  the bone name. */
  std::string node_name = node->name;
  std::map<std::string, uint32_t>::const_iterator 
    item = mesh.bone_name_to_local_id.find(node_name);
  bool is_bone = (item != mesh.bone_name_to_local_id.end()); /* this node is a bone */
  const Bone* bone = (is_bone ? &(mesh.bones[item->second]) : NULL); /* the pointer to the bone */
  bool has_anim = (node->animations.size() > 0); /* this node contains animation(s) */

  /* Compute node transform. */
  Mat4x4 node_transform;
  if (has_anim) {
    const Animation& anim = node->animations[anim_id];
    double anim_tick = play_time * anim.ticks_per_second;
    node_transform = _interpolate_skeletal_animation(anim, anim_tick, this->keyframe_interp_mode);
  }
  else {
    node_transform = node->transform;
  }

  /* Compute accumulated node transform for recursion */
  Mat4x4 accumulated_transform = mul(parent_transform, node_transform);

  /* Compute bone final tranformation matrix and save to uniform variable */
  uint32_t node_unique_id = node_name_to_unique_id[node_name];
  if (is_bone) {
    /* Update bone_matrices (important). */
    bone_matrices[node_unique_id] = mul(accumulated_transform, bone->offset);
    /* In some tutorials, a global inverse transform is applied to the end 
    of the transformation chain, but here I ignore it as apply an additional
    transformation seems to mess up the model location. */
  }

  /* continue to child nodes */
  for (uint32_t i_node = 0; i_node < node->childs.size(); i_node++) {
    _update_mesh_skeletal_animation_from_node(
      node->childs[i_node], accumulated_transform, mesh,
      anim_id, play_time, bone_matrices);
  }
}

template<typename T>
inline T 
_interpolate_key_frames(
  const std::vector<KeyFrame<T>>& key_frames, 
  const double tick,
  const KeyFrameInterpType interp)
{
  uint32_t n_frames = (uint32_t)key_frames.size();

  /* special cases handling */
  if (n_frames == 1)
    return key_frames[0].value;
  if (tick <= key_frames[0].tick)
    return key_frames[0].value;
  if (tick >= key_frames[n_frames - 1].tick)
    return key_frames[n_frames - 1].value;

  /* binary search */
  uint32_t left = 0, right = (uint32_t)key_frames.size() - 1;
  while (left < right) {
    uint32_t mid = (left + right) / 2;
    if (key_frames[mid].tick <= tick && tick < key_frames[mid + 1].tick) {
      /* found */
      left = mid;
      right = mid + 1;
      break;
    }
    else {
      if (key_frames[mid].tick < tick)
        left = mid + 1;
      else
        right = mid;
    }
  }

  /* interpolate left and right using different modes */
  if (interp == KeyFrameInterpType_Nearest) {
    return key_frames[left].value;
  }
  else if (interp == KeyFrameInterpType::KeyFrameInterpType_Linear) {
    /* interpolate left and right (=left+1) */
    double weight = (tick - key_frames[left].tick) /
      (key_frames[right].tick - key_frames[left].tick);
    if (weight < 0.0) weight = 0.0;
    if (weight > 1.0) weight = 1.0;

    return key_frames[left].value * (1 - weight) +
      key_frames[right].value * weight;
  }
  else /* this line should never be run */
    return key_frames[0].value;
}

/* 
Template function specialization for quaternions. 
Since the interpolation for two quaternions cannot simply done by linear 
interpolation, they should use spherical interpolation (aka slerp).
*/
template<>
inline Quat 
_interpolate_key_frames(
  const std::vector<KeyFrame<Quat>>& key_frames,
  const double tick,
  const KeyFrameInterpType interp)
{
  uint32_t n_frames = (uint32_t)key_frames.size();

  /* special cases handling */
  if (n_frames == 1)
    return key_frames[0].value;
  if (tick <= key_frames[0].tick)
    return key_frames[0].value;
  if (tick >= key_frames[n_frames - 1].tick)
    return key_frames[n_frames - 1].value;

  /* binary search */
  uint32_t left = 0, right = (uint32_t)key_frames.size() - 1;
  while (left < right) {
    uint32_t mid = (left + right) / 2;
    if (key_frames[mid].tick <= tick && tick < key_frames[mid + 1].tick) {
      /* found */
      left = mid;
      right = mid + 1;
      break;
    }
    else {
      if (key_frames[mid].tick < tick)
        left = mid + 1;
      else
        right = mid;
    }
  }

  /* interpolate left and right using different modes */
  if (interp == KeyFrameInterpType_Nearest) {
    Quat q1 = key_frames[left].value;
    return normalize(q1);
  }
  else if (interp == KeyFrameInterpType_Linear) {
    /* interpolate left and right (=left+1) */
    double weight = (tick - key_frames[left].tick) / (key_frames[right].tick - key_frames[left].tick);
    if (weight < 0.0) weight = 0.0;
    if (weight > 1.0) weight = 1.0;

    Quat q1 = key_frames[left].value;
    Quat q2 = key_frames[right].value;
    Quat q = slerp(q1, q2, weight);
    return normalize(q);
  }
  else /* this line should never be run */
    return key_frames[0].value;
}

Mat4x4 
Model::_interpolate_skeletal_animation(
  const Animation& anim, const double tick, const KeyFrameInterpType interp)
{
  /*
  Interpolate position, scaling, and rotation.
  NOTE: key frames are sorted by default, and at least one keyframe 
  should exist in the animation (even if the model has no animation).
  */
  Vec3 position = _interpolate_key_frames<Vec3>(anim.position_key_frames, tick, interp);
  Vec3 scaling  = _interpolate_key_frames<Vec3>(anim.scaling_key_frames, tick, interp);
  Quat rotation = _interpolate_key_frames<Quat>(anim.rotation_key_frames, tick, interp);

  /* build matrices and combine them */
  Mat4x4 position_transform(
    1.0, 0.0, 0.0, position.x,
    0.0, 1.0, 0.0, position.y,
    0.0, 0.0, 1.0, position.z,
    0.0, 0.0, 0.0, 1.0
  );
  Mat4x4 scaling_transform(
    scaling.x, 0.0, 0.0, 0.0,
    0.0, scaling.y, 0.0, 0.0,
    0.0, 0.0, scaling.z, 0.0,
    0.0, 0.0, 0.0, 1.0
  );
  Mat4x4 rotation_transform(quat_to_mat3x3(rotation));

  return mul(position_transform, mul(rotation_transform, scaling_transform));
}

void 
Model::_dump_mesh(const Mesh & mesh)
{
  printf("    Total number of vertices: %zu\n", mesh.vertices.size());
  printf("    Total number of indices/tri_faces: %zu/%zu\n", mesh.indices.size(), mesh.indices.size() / 3);
  printf("    Material ID: %u\n", mesh.mat_id);
  this->_dump_material(this->materials[mesh.mat_id]);
  printf("    Number of bones: %zu\n", mesh.bones.size());
}

void 
Model::_dump_material(const Material & material)
{
  /* diffuse texture */
  printf("      Diffuse: \"%s\" (%s)\n", 
    material.diffuse_texture_file.c_str(),
    (material.diffuse_texture.get_pixel_data() != NULL) ? "OK" : "NOT FOUND");
  printf("               size=%dx%d\n", 
    material.diffuse_texture.get_width(), 
    material.diffuse_texture.get_width());
}

void 
Model::_dump_node(const Node* node, const uint32_t indent)
{
  std::string node_name = node->name;
  std::string pad = "";
  for (uint32_t i = 0; i < indent; i++) pad += " ";
  printf("%s%s [node_id=%u]\n", pad.c_str(), node_name.c_str(), node_name_to_unique_id[node_name]);
  for (uint32_t i = 0; i < node->childs.size(); i++) {
    this->_dump_node(node->childs[i], indent + 2);
  }
}

Node*
Model::_find_node_by_name(const std::string & node_name)
{
  std::map<std::string, Node*>::const_iterator item = node_name_to_ptr.find(node_name);
  if (item != node_name_to_ptr.end())
    return item->second;
  else
    return NULL;
}

Animation* 
Model::_find_node_animation_by_name(Node& node, const std::string & anim_name)
{
  for (uint32_t i_anim = 0; i_anim < node.animations.size(); i_anim++) {
    std::string anim_name = node.animations[i_anim].name;
    if (anim_name == anim_name) {
      return &(node.animations[i_anim]);
    }
  }
  return NULL;
}

void 
Model::update_skeletal_animation_for_mesh(const Mesh& mesh,
  const std::string& anim_name, double play_time, Mat4x4* bone_matrices)
{
  /* traverse from root node to calculate all the bone transformations
  for a single mesh and save the calculated results into bone_matrices */
  std::map<std::string, uint32_t>::const_iterator 
    item = anim_name_to_unique_id.find(anim_name);
  if (item == anim_name_to_unique_id.end()) {
    /* The animation being played does not exist. I want to make it a 
    silent fail since this function may be called frequently. Printing 
    an error message could cause a significant performance hit. */
    return;
  }
  uint32_t anim_id = item->second;
  this->_update_mesh_skeletal_animation_from_node(
    root_node, Mat4x4::identity(), mesh, 
    anim_id, play_time, bone_matrices);
}

Vec3 calculate_tangent(
  const Vec3 & p0, const Vec3 & p1, const Vec3 & p2, 
  const Vec2 & t0, const Vec2 & t1, const Vec2 & t2)
{
  /* assume p0-p1-p2 is counter clock wised */
  Vec3 e1 = p1 - p0, e2 = p2 - p0;
  double dU0 = t1.x - t0.x, dU1 = t2.x - t0.x;
  double dV0 = t1.y - t0.y, dV1 = t2.y - t0.y;
  Mat2x2 Q = Mat2x2(
    dU0, dV0,
    dU1, dV1
  );
  Mat2x2 Q_inv = inverse(Q);
  return normalize(Vec3(
    Q_inv.i11 * e1.x + Q_inv.i12 * e2.x,
    Q_inv.i11 * e1.y + Q_inv.i12 * e2.y,
    Q_inv.i11 * e1.z + Q_inv.i12 * e2.z
  ));
}

void calculate_tangent_bitangent(
  const Vec3 & p0, const Vec3 & p1, const Vec3 & p2, 
  const Vec2 & t0, const Vec2 & t1, const Vec2 & t2, 
  Vec3 & tangent, Vec3 & bitangent)
{
  /* assume p0-p1-p2 is counter clock wised */
  Vec3 e1 = p1 - p0, e2 = p2 - p0;
  double dU0 = t1.x - t0.x, dU1 = t2.x - t0.x;
  double dV0 = t1.y - t0.y, dV1 = t2.y - t0.y;
  Mat2x2 Q = Mat2x2(
    dU0, dV0,
    dU1, dV1
  );
  Mat2x2 Q_inv = inverse(Q);
  tangent = normalize(Vec3(
    Q_inv.i11 * e1.x + Q_inv.i12 * e2.x,
    Q_inv.i11 * e1.y + Q_inv.i12 * e2.y,
    Q_inv.i11 * e1.z + Q_inv.i12 * e2.z
  ));
  bitangent = normalize(Vec3(
    Q_inv.i21 * e1.x + Q_inv.i22 * e2.x,
    Q_inv.i21 * e1.y + Q_inv.i22 * e2.y,
    Q_inv.i21 * e1.z + Q_inv.i22 * e2.z
  ));
}

}; /* namespace sgl */
