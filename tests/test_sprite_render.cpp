#include <stdio.h>
#include "sgl.h"

using namespace sgl;

int w = 500, h = 500;

Pipeline pipeline;
Texture tex, checker, chess;
BaseSpriteRenderer renderer;

void init_render() {
  /* create or load existing textures */
  tex = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  checker = sgl::load_texture("textures/checker_256.png", PixelFormat_BGRA8888);
  chess = sgl::load_texture("textures/chess.png", PixelFormat_BGRA8888);

  /* clear texture */
  pipeline.bind_render_target(0, &tex);
  pipeline.clear_render_targets(Vec4(0.5, 0.5, 0.5, 1.0));
  pipeline.set_num_threads(1);

  /* initialize render pipeline */
  renderer.out_texs.color = &tex;
  renderer.pipeline = &pipeline;
}

void render_and_save_to_disk() {
  renderer.run(&checker, Vec2(200, 300), Vec2(0.6, 0.6), 0.85, Vec3(0.8, 0.8, 0.8));
  renderer.run(&checker, Vec2(300, 200), Vec2(0.8, 0.8), -0.2, Vec3(1.0, 1.0, 1.0));
  renderer.run(&chess, Vec2(200, 100), Vec2(3.0, 3.0), 0.0, Vec3(1.0, 1.0, 1.0));
  tex.save_png("test_sprite_render.png");
}

int main(int argc, char* argv[]) {

  set_cwd(gd(argv[0]));  /* set current working directory */
  init_render();
  render_and_save_to_disk();

  return 0;
}
