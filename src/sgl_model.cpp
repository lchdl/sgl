#include "sgl_model.h"
#include "assimp/Importer.hpp"
#include "sgl_math.h"
#include "sgl_utils.h"
#include <string>
#include <vector>

namespace sgl {

Model::Model() {
  _importer = NULL;
  model_transform = Mat4x4::identity();
}
Model::~Model() {
  this->unload();
}

void
Model::unload() {
  if (this->_importer != NULL) {
    delete this->_importer;
    this->_importer = NULL;
  }
  this->_scene = NULL;
  this->meshes.clear();
  this->materials.clear();
}

bool 
Model::load(const std::string& file) {
  
  /* clear trash data from previous load */
  this->unload(); 
  
  /* import model file using Assimp */
  this->_importer = new ::Assimp::Importer();
 	
  /* if the model is packed as a *.zip file, unpack it first */
  std::string temp_folder = "";
  std::string model_file = "";

  if (endswith(file, ".zip")) {
    temp_folder = mktdir(gd(file));
		int zipret = zip_extract(file.c_str(), temp_folder.c_str(), NULL, NULL);
		if (zipret < 0) {
			printf("Assimp import error: cannot unzip file \"%s\".", file.c_str());
			rm(temp_folder);
      return false;
		}
		std::vector<std::string> files = ls(temp_folder);
		for (auto& file : files) {
			size_t dpos = file.find_last_of(".");
			std::string file_no_ext = file.substr(0, dpos);
			std::string file_ext = file.substr(dpos + 1);
			if (endswith(file_no_ext, "model")) {
				if (file_ext == "obj" || file_ext == "md5mesh") {
					model_file = file;
					break;
				}
			}
		}
		if (model_file == "") {
			printf("Assimp import error: you need to provide a file "
          "named \"model.*\" in zipped file \"%s\".", file.c_str());
			rm(temp_folder);
      return false;
		}
  }
  else {
    model_file = file;
  }
	
  /* then import the file using assimp */
  this->_scene = this->_importer->ReadFile(model_file.c_str(),
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);
	if (!_scene || !_scene->mRootNode || _scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		printf("Assimp importer.ReadFile() error when loading file \"%s\": \"%s\".\n",
			file.c_str(), this->_importer->GetErrorString());
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
  
  /* parse meshes */
  uint32_t n_meshes = _scene->mNumMeshes;
  this->meshes.resize(n_meshes);
  const aiVector3D zvec = aiVector3D(0.0, 0.0, 0.0);
  for(uint32_t i_mesh = 0; i_mesh < n_meshes; i_mesh++) {
    const aiMesh* mesh = _scene->mMeshes[i_mesh];
    const uint32_t n_vert = mesh->mNumVertices;  
    /* load vertex (positions, normals, and texture coordinates) */
    for (uint32_t i_vert = 0; i_vert < n_vert; i_vert++) {
      const aiVector3D* position = &mesh->mVertices[i_vert];
      const aiVector3D* normal   = &mesh->mNormals[i_vert];
      const aiVector3D* texcoord = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i_vert] : &zvec;
      Vertex v;
      v.p = Vec3(double(position->x), double(position->y), double(position->z));
      v.n = Vec3(double(normal->x),   double(normal->y),   double(normal->z));
      v.t = Vec2(double(texcoord->x), double(texcoord->y));
      v.bone_IDs = IVec4(-1,-1,-1,-1);
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

    /* read and calculate global inverse transformation matrix */
    this->global_inverse_transform = convert_assimp_mat4x4(
      this->_scene->mRootNode->mTransformation).inverse();

    /* load all the bones */
    for (uint32_t i_bone = 0; i_bone < mesh->mNumBones; i_bone++) {
      Bone bone;
      bone.name = mesh->mBones[i_bone]->mName.data;
      bone.offset_matrix = convert_assimp_mat4x4(
				mesh->mBones[i_bone]->mOffsetMatrix);
      /* number of affected vertices by this bone */
      uint32_t n_bone_verts = mesh->mBones[i_bone]->mNumWeights;
      for (uint32_t i_vert = 0; i_vert < n_bone_verts; i_vert++) {
        /* read and save all info about the affected vertices by this bone */
        aiVertexWeight vw = mesh->mBones[i_bone]->mWeights[i_vert];
        Bone::VertexCtrl vc;
        vc.index = vw.mVertexId;
        vc.weight = vw.mWeight;
        bone.vertices.push_back(vc);
        /* write bone info into affected vertex (let the vertex know
         * there is a bone that influences itself). */
        Vertex& affected_vert = this->meshes[i_mesh].vertices[vc.index];
        _register_vertex_weight(affected_vert, i_bone, vc.weight);
      }
      this->meshes[i_mesh].bone_name_to_id.insert(
        std::pair<std::string, uint32_t>(
          bone.name, uint32_t(this->meshes[i_mesh].bones.size())
        )
      );
      this->meshes[i_mesh].bones.push_back(bone);
    }
  }

  /* parse model animation(s) (if exists) */
  uint32_t n_anims = _scene->mNumAnimations;
  for (uint32_t i_anim = 0; i_anim < n_anims; i_anim++) {
    const aiAnimation* anim = _scene->mAnimations[i_anim];
    double ticks_per_second = anim->mTicksPerSecond;
    if (ticks_per_second < 1.0) ticks_per_second = 25.0;
    uint32_t n_ctrl_bones = anim->mNumChannels; /* number of bones this animation controls */
    std::string anim_name = anim->mName.data;
    /* loop for each bone this animation controls */
    for (uint32_t i_channel = 0; i_channel < n_ctrl_bones; i_channel++) {
      const aiNodeAnim* bone_anim = anim->mChannels[i_channel];
      std::string bone_name = bone_anim->mNodeName.data;
      Bone* bone = _find_bone_by_name(bone_name);
      if (bone != NULL) {
        /* read bone animation key frames */
        Animation* dst_anim = _find_bone_animation_by_name(*bone, anim_name);
        if (dst_anim == NULL) {
          /* create new animation if not exist */
          Animation new_anim;
          new_anim.name = anim_name;
          new_anim.ticks_per_second = ticks_per_second;
          bone->animations.push_back(new_anim);
          dst_anim = &(bone->animations[bone->animations.size() - 1]);
        }
        /* read scaling key frames */
        for (uint32_t i_key = 0; i_key < bone_anim->mNumScalingKeys; i_key++) {
          KeyFrame<Vec3> key_frame;
          key_frame.tick = bone_anim->mScalingKeys[i_key].mTime;
          key_frame.value = convert_assimp_vec3(bone_anim->mScalingKeys[i_key].mValue);
          dst_anim->scaling_key_frames.push_back(key_frame);
        }
        /* read position key frames */
        for (uint32_t i_key = 0; i_key < bone_anim->mNumPositionKeys; i_key++) {
          KeyFrame<Vec3> key_frame;
          key_frame.tick = bone_anim->mPositionKeys[i_key].mTime;
          key_frame.value = convert_assimp_vec3(bone_anim->mPositionKeys[i_key].mValue);
          dst_anim->position_key_frames.push_back(key_frame);
        }
        /* read rotation key frames */
        for (uint32_t i_key = 0; i_key < bone_anim->mNumRotationKeys; i_key++) {
          KeyFrame<Quat> key_frame;
          key_frame.tick = bone_anim->mRotationKeys[i_key].mTime;
          key_frame.value = convert_assimp_quat(bone_anim->mRotationKeys[i_key].mValue);
          dst_anim->rotation_key_frames.push_back(key_frame);
        }
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
        this->materials[i_mat].diffuse_texture = load_texture(tex_full_path);
        if (this->materials[i_mat].diffuse_texture.pixels == NULL) {
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
  return true;
}

void 
Model::_register_vertex_weight(
    Vertex& v, 
    uint32_t bone_index, 
    double weight) 
{
  /* insert & sort vertex weights in descent order,
   * in this way, only top-k bones will be kept for
   * each vertex. */
  bool registered = false;
  for (uint32_t i=0; i<MAX_BONES_INFLUENCE_PER_VERTEX; i++) {
    if (weight > v.bone_weights[i]) {
      /* insert bone to this slot */
      registered = true;
      /* shift right */
      for (uint32_t j=MAX_BONES_INFLUENCE_PER_VERTEX-1; j>i; j--) {
        v.bone_weights[j] = v.bone_weights[j-1];
        v.bone_IDs[j] = v.bone_IDs[j-1];
      }
      /* insert */
      v.bone_weights[i] = weight;
      v.bone_IDs[i] = bone_index;
      break;
    }
  }
  if (registered == false) {
    /* All 4 slots have been occupied, we print a warning to let user
     * know and then continue. */
    printf("[*] Warning: Cannot register vertex weight (bone_index=%d, "
           "weight=%.4lf), all 4 slots have been occupied. Ignored.\n", 
           bone_index, weight);
  }
}

void Model::_update_bone_matrices_from_node(
  const aiNode* node, /* current node being traversed */
  const Mat4x4& parent_transform, /* accumulated parent node transformation matrix */
  const Mesh& mesh, /* mesh that contains all the bones */
  Uniforms& uniforms)
{
  std::string node_name = node->mName.data;
  Mat4x4 node_transform = convert_assimp_mat4x4(node->mTransformation);
  Mat4x4 accumulated_transform = mul(parent_transform, node_transform);
  
  /* 
  in Assimp, if a node is actually a bone, then the node name will be set
  to be the same as the bone name. 
  */
  std::map<std::string, uint32_t>::const_iterator item = mesh.bone_name_to_id.find(node_name);
  if (item != mesh.bone_name_to_id.end()){
    uint32_t bone_index = item->second;
    uniforms.bone_matrices[bone_index] = mul(this->global_inverse_transform, 
        mul(accumulated_transform, mesh.bones[bone_index].offset_matrix));
  }

  for (uint32_t i_node = 0; i_node < node->mNumChildren; i_node++) {
    _update_bone_matrices_from_node(node->mChildren[i_node], accumulated_transform, mesh, uniforms);
  }
  /* after this function returns all bone_matrices will be updated
   * and ready for the subsequent draw call */  
}

Bone*
Model::_find_bone_by_name(const std::string & name)
{
  for (uint32_t i_mesh = 0; i_mesh < this->meshes.size(); i_mesh++) {
    Mesh& mesh = this->meshes[i_mesh];
    std::map<std::string, uint32_t>::const_iterator item = mesh.bone_name_to_id.find(name);
    if (item != mesh.bone_name_to_id.end()) {
      uint32_t bone_index = item->second;
      return &(mesh.bones[bone_index]);
    }
  }
  return NULL;
}

Animation * Model::_find_bone_animation_by_name(
  Bone& bone, const std::string & name)
{
  for (uint32_t i_anim = 0; i_anim < bone.animations.size(); i_anim++) {
    std::string anim_name = bone.animations[i_anim].name;
    if (anim_name == name) {
      return &(bone.animations[i_anim]);
    }
  }
  return NULL;
}

void Model::update_bone_matrices_for_mesh(
  uint32_t i_mesh,
  Uniforms& uniforms)
{
  Mesh& mesh = this->meshes[i_mesh];
  this->_update_bone_matrices_from_node(_scene->mRootNode, Mat4x4::identity(), mesh, uniforms);
}

void
model_VS(const Vertex &vertex_in, const Uniforms &uniforms, 
    Vertex_gl& vertex_out) { 
  /* uniforms:
   * in_textures[0]: diffuse texture.
   * */
  const Mat4x4 &model = uniforms.model;
  const Mat4x4 &view = uniforms.view;
  const Mat4x4 &projection = uniforms.projection;
  Mat4x4 transform = mul(projection, mul(view, model));

  if (vertex_in.bone_IDs.i[0] < 0) {
    /* vertex does not belong to any bone */
    Vec4 gl_Position = mul(transform, Vec4(vertex_in.p, 1.0));
    vertex_out.gl_Position = gl_Position;
    vertex_out.t = vertex_in.t;
    vertex_out.wn = mul(model, Vec4(vertex_in.n, 1.0)).xyz();
    vertex_out.wp = mul(model, Vec4(vertex_in.p, 1.0)).xyz();
  }
  else {
		/* vertex is controlled by at least one bone */
    /* calculate:
     * p_final = sum( w[i]*m[i]*p, for i in [0,1,2,3] ), where
     * p is the vertex position in local model space (T-pose),
     * m[i] is the i-th final bone transformation matrix,
     * w[i] is the i-th bone influence weight to the vertex.
     * to make computation a little bit faster, we calculate
     * w[i]*m[i] for i in [0,1,2,3], then multiply it with p. */
    Mat4x4 final_matrix;
    for (uint32_t i_bone=0; 
         i_bone<MAX_BONES_INFLUENCE_PER_VERTEX; 
         i_bone++) 
    {
      int32_t bone_id = vertex_in.bone_IDs.i[i_bone];
      /* bone_id can be negative, which indicates that the
       * corresponding slot is unused. */
      if (bone_id < 0) break; 
      double bone_weight = vertex_in.bone_weights.i[i_bone];
      const Mat4x4& bone_matrix = uniforms.bone_matrices[bone_id];
      final_matrix += bone_weight * bone_matrix;
    }
    /* apply final matrix to vertex position */
    Vec4 p_rig = mul(final_matrix, Vec4(vertex_in.p, 1.0));
    Vec4 n_rig = mul(final_matrix, Vec4(vertex_in.n, 1.0));
    vertex_out.gl_Position = mul(transform, p_rig);
    vertex_out.t = vertex_in.t;
    vertex_out.wn = mul(model, n_rig).xyz();
    vertex_out.wp = mul(model, p_rig).xyz();
  }

}

void 
model_FS(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard) {
  Vec2 uv = Vec2(fragment_in.t.x, 1.0 - fragment_in.t.y);
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  color_out = Vec4(textured, 1.0);
}

}; /* namespace sgl */
