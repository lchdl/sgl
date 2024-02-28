#include "sgl_model.h"
#include "assimp/Importer.hpp"
#include <vector>

namespace sgl {

Mesh::Mesh() { 
  importer = NULL;
}
Mesh::~Mesh() {
}

void
Mesh::unload() {
  this->parts.clear();
  this->textures.clear();
}

bool 
Mesh::load(const std::string& file) {
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
			printf("Assimp import error: you need to provide a file named \"model.*\" in "
				"zipped file \"%s\".", file.c_str());
			rm(temp_folder);
      return false;
		}
  }
  else {
    model_file = file;
  }
	/* then import the file using assimp */
  const aiScene* scene = this->importer->ReadFile(model_file.c_str(),
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
		aiProcess_JoinIdenticalVertices);
	if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
		printf("Assimp importer.ReadFile() error when loading file \"%s\": \"%s\".\n",
			file.c_str(), this->importer->GetErrorString());
		if (temp_folder != "")
      rm(temp_folder);
    return false;
	}
  /* parse scene to mesh */
  uint32_t n_parts = scene->mNumMeshes;
  this->parts.resize(n_parts);
  const aiVector3D zvec = aiVector3D(0.0, 0.0, 0.0);
  for(uint32_t i_part = 0; i_part < n_parts; i_part++) {
    /* initialize each mesh part */
    const aiMesh* part = scene->mMeshes[i_part];
    const uint32_t n_vert = part->mNumVertices;  
    for (uint32_t i_vert = 0; i_vert < n_vert; i_vert++) {
      /* load vertex (positions, normals, and texture coordinates) */
      const aiVector3D* position = &part->mVertices[i_vert];
      const aiVector3D* normal   = &part->mNormals[i_vert];
      const aiVector3D* texcoord = part->HasTextureCoords(0) ? &part->mTextureCoords[0][i_vert] : &zvec;
      Vertex v;
      v.p = Vec3(double(position->x), double(position->y), double(position->z));
      v.n = Vec3(double(normal->x), double(normal->y), double(normal->z));
      v.t = Vec2(double(texcoord->x), double(texcoord->y));
      this->parts[i_part].vertices.push_back(v);
    }
    for (uint32_t i_face = 0; i_face < part->mNumFaces; i_face++) {
      /* load triangle face indices */
      const aiFace& face = part->mFaces[i_face];
      this->parts[i_part].indices.push_back(face.mIndices[0]);
      this->parts[i_part].indices.push_back(face.mIndices[1]);
      this->parts[i_part].indices.push_back(face.mIndices[2]);
    }
  }
  for (uint32_t i_mat = 0; i_mat < scene->mNumMaterials; i_mat++) {
    /* load materials */
    const aiMaterial* material = scene->mMaterials[i_mat];
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0){
      aiString texpath;
      if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texpath, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
        printf("%s\n", texpath.data);

        /*std::string FullPath = Dir + "/" + texpath.data;
        m_Textures[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());
        if (!m_Textures[i]->Load()) {
          printf("Error loading texture '%s'\n", FullPath.c_str());
          delete m_Textures[i];
          m_Textures[i] = NULL;
          Ret = false;
        }*/
      }
      else{
        printf("Error on loading mesh materials: cannot get diffuse texture.");
      }
    }
  }
  /* parse ended, now cleaning up... */
  /* if model is loaded from an unpacked zip file, remove the temporary dir. */
  if (temp_folder != "")
    rm(temp_folder);
  /* delete the importer since we don't need it anymore outside load() */
  delete this->importer;
  this->importer = NULL;
  return true;
}


}; /* namespace sgl */
