#include <stdio.h>
#include "sgl.h"

using namespace sgl;

int main(int argc, char* argv[]) {
  set_cwd(gd(argv[0]));  /* set current working directory */

  if (!sgl::Audio::initialize_audio())
    exit(1);

  sgl::Audio::Sound snd;
  snd.load("assets/common/audios/rain.mp3");
  snd.play();

  printf("press any key...\n");
  getchar();

  return 0;
}
