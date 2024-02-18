#pragma once
#include "sgl_math.h"
#include "sgl_texture.h"

namespace sgl {

const int MAX_TEXTURES_PER_DRAW_CALL = 16;
const int MAX_TEXTURES_PER_SHADING_UNIT = 8;

struct RenderConfig {
  /* texture array in each draw call */
  Texture *textures[MAX_TEXTURES_PER_DRAW_CALL];
  /* which texture will be used for color component output */
  int color_texture_id;
  /* which texture will be used for depth component output */
  int depth_texture_id;
  /* which textures will be registered to uniform variables. */
  int uniform_texture_ids[MAX_TEXTURES_PER_SHADING_UNIT];

  Mat4x4 model_transform;

  /* camera/eye settings */
  struct {
    Vec3 position;
    Vec3 look_at;
    Vec3 up_dir;
    struct {
      bool enabled;
      double near, far, field_of_view;
    } perspective;
    struct {
      bool enabled;
      double width, height, depth;
    } orthographic;
  } eye;

  /* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

  RenderConfig() {
    for (int i = 0; i < MAX_TEXTURES_PER_DRAW_CALL; i++) textures[i] = NULL;
    color_texture_id = 0;
    depth_texture_id = 0;
    for (int i = 0; i < MAX_TEXTURES_PER_SHADING_UNIT; i++)
      uniform_texture_ids[i] = 0;

    eye.look_at = Vec3(0, 0, 0);
    eye.position = Vec3(10, 10, 10);
    eye.up_dir = Vec3(0, 1, 0);

    eye.perspective.enabled = true;
    eye.perspective.near = 0.1;
    eye.perspective.far = 100.0;
    eye.perspective.field_of_view = PI / 4.0;

    eye.orthographic.enabled = false;
    eye.orthographic.width = 256.0;
    eye.orthographic.height = 256.0;
    eye.orthographic.depth = 256.0;

    model_transform = Mat4x4::identity();
  }
};

};   // namespace sgl
