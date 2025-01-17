#include <stdio.h>
#include "sgl.h"

using namespace sgl;

int w = 500, h = 500;

Texture tex, checker_256x256, checker_5x5, checker_pattern, 
  chess, chess_mask;
SpriteRenderer renderer;

void init_render() {
  /* create or load existing textures */
  tex = sgl::create_texture(w, h, PixelFormat_BGRA8888, TextureSampling_Nearest, TextureUsage_ColorComponents);
  checker_256x256 = sgl::load_texture("assets/common/textures/checker_256.png", PixelFormat_BGRA8888);
  checker_5x5 = sgl::load_texture("assets/common/textures/checker_5x5.png", PixelFormat_BGRA8888);
  checker_pattern = sgl::load_texture("assets/common/textures/checker_pattern.png", PixelFormat_BGRA8888);
  chess = sgl::load_texture("assets/common/textures/chess.png", PixelFormat_BGRA8888);
  chess_mask = sgl::load_texture("assets/common/textures/chess_mask.png", PixelFormat_BGRA8888);

  /* initialize render pipeline */
  renderer.bind_render_target(&tex);
  renderer.clear_render_target(Vec4(0.5, 0.5, 0.5, 1.0));
  renderer.set_num_threads(2);
}

void render_and_save_to_disk() {

  /* Demonstrates various methods for rendering a sprite onto a texture. */

  renderer.draw(&checker_256x256, Vec2(200, 300), Vec2(0.6, 0.6), 0.85, Vec3(0.8, 0.8, 0.8));
  renderer.draw(&checker_256x256, Vec2(300, 200), Vec2(0.8, 0.8), -0.2, Vec3(1.0, 1.0, 1.0));
  renderer.draw(&chess, Vec2(200, 100), Vec2(3.0, 3.0), 0.0, Vec3(1.0, 1.0, 1.0));
  renderer.set_sprite_origin_mode(SpriteOriginMode_BottomLeft);
  renderer.draw(&checker_5x5, Vec2(0, 0), Vec2(3.0, 3.0), 0.0, Vec3(1.0, 0.0, 0.0));
  renderer.set_sprite_origin_mode(SpriteOriginMode_BottomRight);
  renderer.draw(&checker_5x5, Vec2(w, 0), Vec2(3.0, 3.0), 0.0, Vec3(0.0, 1.0, 0.0));
  renderer.set_sprite_origin_mode(SpriteOriginMode_TopLeft);
  renderer.draw(&checker_5x5, Vec2(0, h), Vec2(3.0, 3.0), 0.0, Vec3(0.0, 0.0, 1.0));
  renderer.set_sprite_origin_mode(SpriteOriginMode_TopRight);
  renderer.draw(&checker_5x5, Vec2(w, h), Vec2(3.0, 3.0), 0.0, Vec3(1.0, 1.0, 1.0));

  renderer.set_sprite_origin_mode(SpriteOriginMode_BottomLeft);
  renderer.draw(&checker_pattern, 2, 1, 3, 3, 0, 100, Vec2(1.0, 1.0), 0, Vec3(1.0, 1.0, 1.0));
  renderer.set_sprite_origin_mode(SpriteOriginMode_Center);
  renderer.draw(&checker_pattern, 2, 1, 3, 3, 50, 200, Vec2(10.0, 10.0), sgl::PI/8, Vec3(1.0, 1.0, 1.0));

  renderer.set_sprite_origin_mode(SpriteOriginMode_BottomLeft);
  renderer.draw(&chess, 33, 13, 14, 19, 100, 300, Vec2(2.0, 2.0), 0.0, Vec3(1.0, 1.0, 1.0), &chess_mask);

  blit_texture(&chess, &tex, 81, 6, 14, 26, 200, 300, Vec2(2.0, 2.0), -sgl::PI / 8, Vec3(1.0, 1.0, 1.0), SpriteOriginMode_TopLeft);

  tex.save_png("test_sprite_render.png");
}

int main(int argc, char* argv[]) {

  set_cwd(gd(argv[0]));  /* set current working directory */
  init_render();
  render_and_save_to_disk();

  return 0;
}
