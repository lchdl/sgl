#include "sgl_model.h"
#include "assimp/Importer.hpp"
#include <string>
#include <vector>

namespace sgl {

Model::Model() {
  importer = NULL;
  transform = Mat4x4::identity();
}
Model::~Model() {
  this->unload();
}

void
Model::unload() {
  if (this->importer != NULL) {
    delete this->importer;
    this->importer = NULL;
  }
  this->scene = NULL;
  this->meshes.clear();
  this->materials.clear();
}

bool 
Model::load(const std::string& file) {
  /* clear trash data from previous load */
  this->unload(); 
  /* import model file using Assimp */
  this->importer = new ::Assimp::Importer();
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
				if (file_ext != "mtl") {
					/* ignore wavefront OBJ mtl file */
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
  this->scene = this->importer->ReadFile(model_file.c_str(),
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		printf("Assimp importer.ReadFile() error when loading file \"%s\": \"%s\".\n",
			file.c_str(), this->importer->GetErrorString());
		if (temp_folder != "")
      rm(temp_folder);
    return false;
	}
  /* In Assimp, a scene consists of multiple meshes, each mesh can 
   * only have one material. If a mesh uses multiple materials for 
   * its surface, it will be split up to multiple sub-meshes so 
   * that each sub-mesh only uses one material. Here we load all 
   * sub-meshes in a scene, and each sub-mesh in Assimp will be 
   * considered as a `mesh part` in here. Please note the slight 
   * difference between the definition of mesh in Assimp and here.
   * */
  /* parse scene to mesh */
  uint32_t n_meshes = scene->mNumMeshes;
  this->meshes.resize(n_meshes);
  const aiVector3D zvec = aiVector3D(0.0, 0.0, 0.0);
  for(uint32_t i_mesh = 0; i_mesh < n_meshes; i_mesh++) {
    /* initialize each mesh part */
    const aiMesh* mesh = scene->mMeshes[i_mesh];
    const uint32_t n_vert = mesh->mNumVertices;  
    for (uint32_t i_vert = 0; i_vert < n_vert; i_vert++) {
      /* load vertex (positions, normals, and texture coordinates) */
      const aiVector3D* position = &mesh->mVertices[i_vert];
      const aiVector3D* normal   = &mesh->mNormals[i_vert];
      const aiVector3D* texcoord = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i_vert] : &zvec;
      Vertex v;
      v.p = Vec3(double(position->x), double(position->y), double(position->z));
      v.n = Vec3(double(normal->x),   double(normal->y),   double(normal->z));
      v.t = Vec2(double(texcoord->x), double(texcoord->y));
      this->meshes[i_mesh].vertices.push_back(v);
    }
    for (uint32_t i_face = 0; i_face < mesh->mNumFaces; i_face++) {
      /* load triangle face indices */
      const aiFace& face = mesh->mFaces[i_face];
      this->meshes[i_mesh].indices.push_back(face.mIndices[0]);
      this->meshes[i_mesh].indices.push_back(face.mIndices[1]);
      this->meshes[i_mesh].indices.push_back(face.mIndices[2]);
    }
    this->meshes[i_mesh].mat_id = mesh->mMaterialIndex;
    for (uint32_t i_bone = 0; i_bone < mesh->mNumBones; i_bone++) {
      /* load bones (here we just load it first, we will parse the
       * bone info later). */
      Bone bone;
      bone.name = mesh->mBones[i_bone]->mName.data;
      /* number of affected vertices by this bone */
      uint32_t n_bone_verts = mesh->mBones[i_bone]->mNumWeights;
      for (uint32_t i_vert = 0; i_vert < n_bone_verts; i_vert++) {
        /* read and save all info about the affected vertices by this bone */
        aiVertexWeight vw = mesh->mBones[i_bone]->mWeights[i_vert];
        Bone::VertexCtrl vc;
        vc.index = vw.mVertexId;
        vc.weight = vw.mWeight;
        bone.vertices.push_back(vc);
        /* write bone info into affected vertex */
        Vertex& affected_vert = this->meshes[i_mesh].vertices[vc.index];
        _register_vertex_weight(affected_vert, vc.index, vc.weight);
      }
    }
  }
  uint32_t n_materials = scene->mNumMaterials;
  this->materials.resize(n_materials);
  for (uint32_t i_mat = 0; i_mat < n_materials; i_mat++) {
    /* load materials */
    const aiMaterial* material = scene->mMaterials[i_mat];
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
  /* insert & sort vertex weights in descent order */
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
    /* all 4 slots have been occupied */
    printf("[*] Cannot register vertex weight: all slots have been occupied.\n");
  }
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
  Mat4x4 transform = mul(mul(projection, view), model);
  Vec4 gl_Position = mul(transform, Vec4(vertex_in.p, 1.0));
  vertex_out.gl_Position = gl_Position;
  vertex_out.t = vertex_in.t;
  vertex_out.wn = mul(model, Vec4(vertex_in.n, 1.0)).xyz();
  vertex_out.wp = mul(model, Vec4(vertex_in.p, 1.0)).xyz();
}

void 
model_FS(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard) {
  Vec2 uv = fragment_in.t;
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  color_out = Vec4(textured, 1.0);
  //color_out = Vec4(1.0,1.0,1.0,1.0);
}

}; /* namespace sgl */
