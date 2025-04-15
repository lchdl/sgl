#pragma once
#include <string>
#include "miniaudio/miniaudio.h"
#include "sgl_utils.h"

namespace sgl {
namespace Audio {

bool initialize_audio();

class Sound : public sgl::NonCopyable {
protected:
  ma_sound _ma_sound_obj;
public:
  bool load(const std::string& file);
  void unload();
  void play();
};

};
};
