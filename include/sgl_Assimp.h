#pragma once

#include <string>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "sgl_utils.h"
#include "zip.h"

namespace sgl {
namespace Assimp {

/**
Wraps up the Assimp scene loading function.
Now load_model() also supports loading *.zip files.
**/
const aiScene* load_model(const std::string& file);

};
};
