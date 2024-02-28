#include "sgl_model.h"
#include "assimp/Importer.hpp"
#include <string>
#include <vector>

namespace sgl {

Mesh::Mesh() { 
  importer = NULL;
  transform = Mat4x4::identity();
}
Mesh::~Mesh() {
}

void
Mesh::set_transform(const Mat4x4& transform) {
  this->transform = transform;
}

void
Mesh::unload() {
  this->parts.clear();
  this->materials.clear();
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
  /* In Assimp, a scene consists of multiple meshes, each mesh can 
   * only have one material. If a mesh uses multiple materials for 
   * its surface, it will be split up to multiple sub-meshes so 
   * that each sub-mesh only uses one material. Here we load all 
   * sub-meshes in a scene, and each sub-mesh in Assimp will be 
   * considered as a `mesh part` in here. Please note the slight 
   * difference between the definition of mesh in Assimp and here.
   * */
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
      v.n = Vec3(double(normal->x),   double(normal->y),   double(normal->z));
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
    this->parts[i_part].mat_id = part->mMaterialIndex;
  }
  uint32_t n_materials = scene->mNumMaterials;
  this->materials.resize(n_materials);
  for (uint32_t i_mat = 0; i_mat < n_materials; i_mat++) {
    /* load materials */
    const aiMaterial* material = scene->mMaterials[i_mat];
    /* load diffuse texture (if exists) */
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0){
      aiString _tp;
      if (material->GetTexture(aiTextureType_DIFFUSE, 0, &_tp, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
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
          printf("Texture loading error: cannot load texture \"%s\". File not exist or have no access.\n", tex_full_path.c_str());
        }
      }
    }
    /* TODO: load other types of textures (if exists) */
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

void
Mesh::draw(Pipeline& ppl, Pass& pass) const {
  /* draw the entire using the given pipeline object and pass object. */
  /* the mesh object will first register its VS and FS to the pipeline, 
   * then the pass object will provide some other important information
   * that is relevant for software rendering (such as providing the 
   * camera info and uniform variables). */
  
  /* Here we will not check the validity of VS and FS (!=NULL) to
   * maximize efficiency. */
  ppl.set_FS(mesh_FS);
  ppl.set_VS(mesh_VS);
  /* setup uniform variables */
  /* passing model transformation matrix. */
  pass.model_transform = this->transform;
  /* since each mesh part will probably have different materials and 
   * only one material can be used when rendering, so we need to use
   * a loop to iterate through all mesh parts. */
  for (uint32_t i_part = 0; i_part < this->parts.size(); i_part++) {
    const MeshPart& part = this->parts[i_part];
    const Material& mat  = this->materials[part.mat_id];
    /* passing texture resources for VS and FS. */
    pass.in_textures[0] = &mat.diffuse_texture;
    for(uint32_t i_tex = 1; i_tex < MAX_TEXTURES_PER_SHADING_UNIT; i_tex++)
      pass.in_textures[i_tex] = NULL;
    /* draw mesh part */
    ppl.draw(part.vertices, part.indices, pass);
  }
}

void
mesh_VS(const Vertex &vertex_in, const Uniforms &uniforms, 
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
mesh_FS(const Fragment_gl &fragment_in, const Uniforms &uniforms,
                     Vec4 &color_out, bool& discard) {
  Vec2 uv = fragment_in.t;
  Vec3 wp = fragment_in.wp;
  Vec3 textured = texture(uniforms.in_textures[0], uv).xyz();
  Vec3 color = (wp + 2.0) / 3.0;
  color_out = Vec4(color * textured, 1.0);
}

}; /* namespace sgl */
