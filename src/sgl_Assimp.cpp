#include "sgl_Assimp.h"
#include <vector>

namespace sgl {
namespace Assimp {

const aiScene* load_model(const std::string& file) {

	if (endswith(file, ".zip")) {
		/* unzip file in a temp folder then load, after loading, delete the folder. */
		const std::string temp_folder = mktdir(gd(file));
		int zipret = zip_extract(file.c_str(), temp_folder.c_str(), NULL, NULL);
		if (zipret < 0) {
			printf("Assimp import error: cannot unzip file \"%s\".", file.c_str());
			return NULL;
		}
		std::vector<std::string> files = ls(temp_folder);
		std::string model_file = "";
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
			return NULL;
		}
		const aiScene* scene = sgl::Assimp::load_model(model_file);
		rm(temp_folder);
		return scene;
	}
	else {
		::Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(file.c_str(),
			aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs |
			aiProcess_JoinIdenticalVertices);
		if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
			printf("Assimp importer.ReadFile() error when loading file \"%s\": \"%s\".\n",
				file.c_str(), importer.GetErrorString());
			return NULL;
		}
		return scene;
	}

}

};
};
