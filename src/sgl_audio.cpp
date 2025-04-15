#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#include "sgl_audio.h"
#include "sgl_utils.h"

namespace sgl {
namespace Audio {

ma_engine engine_obj;

bool sgl::Audio::initialize_audio() {
  return ma_engine_init(NULL, &engine_obj) == MA_SUCCESS;
}

bool Sound::load(const std::string& file) {
  ma_result result = ma_sound_init_from_file(&engine_obj, file.c_str(), 0, NULL, NULL, &_ma_sound_obj);
  return result == MA_SUCCESS;
}
void Sound::unload() {
  ma_sound_uninit(&_ma_sound_obj);
}
void Sound::play() {
  ma_sound_start(&_ma_sound_obj);
}

}; /* namespace Audio */
}; /* namespace sgl */
