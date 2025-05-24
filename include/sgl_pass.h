#pragma once

#include "sgl_pipeline.h"

namespace sgl {

/**

A `pass` is an object that describes a complete render operation
and stores all the resources used during rendering. 

* The reason I introduce the concept of `pass` is that drawing an
object onto the screen correctly requires a lot of preparation
work beforehand, including but not limited to shader initialization,
buffer preparation, uniform variable assignment, etc. To be honest,
many things can go wrong here if not enough attention is paid, and
usually, a blank screen will be shown if there is any bug in your
code, which is not very informative for graphical debugging and can
lower your efficiency. So, wrapping the above process into a `pass`
can standardize the whole process for us, which will be much more
convenient when drawing something complex onto the screen.

**/

class View {
public:
  struct {
    Vec3 position; /* eye position */
    Vec3 look_at;  /* view target */
    Vec3 up_dir;   /* up normal */
    struct {
      bool enabled;
      double near, far, field_of_view;
    } perspective;
    struct {
      bool enabled;
      double near, far, width, height;
    } orthographic;
  } eye;

protected:
  void _build_local_axis(Vec3 & front, Vec3 & up, Vec3 & left);

public:
  /* utility functions */
  Mat4x4 get_view_matrix() const;
  Mat4x4 get_projection_matrix(int w, int h) const;

  /* scene navigation */
  void move_up(double amount);
  void move_down(double amount);
  void move_left(double amount);
  void move_right(double amount);
  void move_forward(double amount);
  void move_backward(double amount);
  void rotate_left(double degrees);
  void rotate_right(double degrees);
  void rotate_up(double degrees);
  void rotate_down(double degrees);

  /* default ctor & dtor */
  View();
  virtual ~View() {}
};

/**
TemplatePass:

A standard template for fully utilizing the programmable pipeline feature
of SGL. The implementation of this class can also serve as a tutorial.
**/
class TemplatePass {
public:
  struct Uniforms {
    /*
    Define your own uniform variables here.
    */
  };
  struct VS_IN : public IVertex {
    /*
    Define your own input format for each vertex here.
    NOTE: All vertex formats should inherit from the IVertex interface.
    The class name doesn't have to be VS_IN; it's used here just for
    demonstration.
    */
  };
  typedef struct VS_OUT : public IFragment {
    /*
    The vertex shader output format. This format is also used as the
    input of the fragment shader.
    NOTE: All vertex output formats / fragment shader input formats
    should inherit from IFragment interface.
    */
    /*
    The Fragment class needs to define how two fragments should be interpolated
    by implementing the following member functions. This process is similar to
    OpenGL's internal interpolation operation on two fragments before running
    the fragment shader. Some attributes need to be linearly interpolated, while
    others are marked with the flat qualifier. For more information about the
    flat qualifier used in OpenGL, see
    https://stackoverflow.com/questions/27581271/flat-qualifier-in-glsl.
    */
    void operator*=(const double& scalar) {
      this->gl_Position *= scalar;
      /* Provide implementation for other attributes here. */
    }
    VS_OUT operator*(const double& scalar) const {
      VS_OUT out;
      out.gl_Position = this->gl_Position * scalar;
      /* Provide implementation for other attributes here. */
      return out;
    }
    VS_OUT operator+(const VS_OUT& frag) const {
      VS_OUT out;
      out.gl_Position = this->gl_Position + frag.gl_Position;
      /* Provide implementation for other attributes here. */
      return out;
    }
  } FS_IN;
  struct Shader {
    /*
    Define the actual shader. A shader must have the following two member
    functions, `VS` and `FS`. `VS` stands for vertex shader and `FS` stands
    for fragment shader. The prototypes of these functions are shown below.
    NOTE: The prototypes of these member functions are fixed and cannot be
    changed.
    */
    void VS(const Uniforms& uniforms, const VS_IN& vertex_in,
      VS_OUT& vertex_out, Vec4& gl_Position) const {
      /*
      Define your own vertex shader here. The `VS_IN` and `VS_OUT` here
      are data structures that you defined previously.
      NOTE: `gl_Position` must be properly set, otherwise the vertex shader
      will not work.
      */
    }
    void FS(const Uniforms& uniforms, const FS_IN& fragment_in, const Vec4& gl_FragCoord,
      FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const {
      /*
      Define your own fragment shader here.
      */
    }
  };
protected:
  typedef Pipeline<Uniforms, VS_IN, FS_IN, Shader> Pipeline_t;
  /*
  Using the defined classes to instantiate the `Pipeline` template. The
  pipeline object instantiated here utilizes all the previously defined
  classes.
  */
  Pipeline_t pipeline;
  Shader       shader;
  Uniforms   uniforms;
  /* TODO: add other member variables here. */
public:
  void draw(/* ... */) {
    Pipeline_t::VertexBuffer_t vertex_buffer;
    Pipeline_t::IndexBuffer_t index_buffer;
    /* ... fill vertex buffer here ... */
    vertex_buffer.push_back(VS_IN(/* ... */));
    /* ... fill index buffer here ... */
    index_buffer.push_back(0 /* int32_t */);
    /* ... fill uniform variables here ... */
    /* finally, draw trianles using the pipeline */
    pipeline.draw(shader, vertex_buffer, index_buffer, uniforms);
  }
};

/**
AnimatedModelRenderer:

Simply draw a model (probably with animation) onto screen.
* Note that a model can consists of multiple meshes, so this class also 
  wraps up multiple draw calls to fully render a model, each draw call
  only renders a single mesh.
**/
class AnimatedModelRenderer
{
public:
  struct Uniforms {
    Mat4x4 world;
    Mat4x4 view;
    Mat4x4 projection;
    /* texture objects */
    const Texture *in_textures[MAX_TEXTURES_PER_SHADING_UNIT];
    /* final bone transformations */
    Mat4x4 bone_matrices[sgl::Model::MAX_NODES_PER_MODEL];
  };
  struct VS_IN : public IVertex {
    Vec3 p; /* vertex position (in model local space) */
    Vec3 n; /* vertex normal (in model local space) */
    Vec2 t; /* vertex texture coordinate */
    /* for skeletal animations */
    IVec4 bone_IDs; /* bones up to 4 */
    Vec4  bone_weights;
  };
  typedef struct VS_OUT : public IFragment {
    Vec3 wp; /* world position */
    Vec3 wn; /* world normal */
    Vec2 t;  /* texture coordinates */

    /* The `Fragment` class need define how two fragments should be interpolated. */
    void  operator*=(const double& scalar);
    VS_OUT operator*(const double& scalar) const;
    VS_OUT operator+(const VS_OUT& frag) const;

  } FS_IN;
  class Shader {
  public:
    void VS(const Uniforms& uniforms, const VS_IN& vertex_in, VS_OUT& vertex_out, Vec4& gl_Position) const;
    void FS(const Uniforms& uniforms, const FS_IN& fragment_in, const Vec4& gl_FragCoord, FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const;
  };

public:
  void                       draw();
  void             load_model_zip(const std::string & zip_file, const std::string& model_fname);
  void        set_model_transform(const Mat4x4& transform);
  PipelineDrawMode  get_draw_mode() const;
  void              set_draw_mode(PipelineDrawMode draw_mode);
  bool get_backface_culling_state() const;
  void set_backface_culling_state(bool state);
  void   set_pipeline_num_threads(int num_threads);
  void             play_animation(const std::string& anim_name, const double& play_time);
  double     query_last_draw_time() const;
  void        bind_render_targets(Texture* color, Texture* depth, Texture* normal);
  void       clear_pipeline_cache();
  void       clear_render_targets(const Vec4& clear_color);
  void        set_default_texture(const char* path);
  void                   set_view(View* view);

public:
  AnimatedModelRenderer();
  virtual ~AnimatedModelRenderer() {}

protected:
  VS_IN _convert_from_mesh_vertex(const Vertex_pnt_nm_bone& v) const;

protected:
  typedef Pipeline<Uniforms, VS_IN, VS_OUT, Shader> Pipeline_t;
  Pipeline_t pipeline;
  Uniforms   uniforms;
  Shader       shader;
  Model         model;
  std::map<uint32_t, Pipeline_t::VertexBuffer_t> vertices_map;
  std::map<uint32_t, Pipeline_t::IndexBuffer_t>   indices_map;

  std::string anim_name; /* name of the current animation being played */
  double      play_time; /* time value for controlling the skeletal animation (in sec.) */
  double last_draw_time; /* draw time (sec) of the last frame */

  /* some model does not have any texture, in this case the default texture is needed */
  sgl::Texture default_texture;

  View* view;
};

class SpriteRenderer {
public:
  struct Uniforms {
    Vec3 color_mask;
    Mat4x4 transform;
    const Texture* in_texture;
    const Texture* in_mask;
  };
  struct VS_IN : public IVertex {
    Vec2 xy, uv; /* position & texcoords */
  };
  typedef struct VS_OUT : public IFragment {
    Vec2 uv;
    void operator*=(const double& scalar) {
      this->uv *= scalar;
      this->gl_Position *= scalar;
    }
    VS_OUT operator*(const double& scalar) const {
      VS_OUT out;
      out.gl_Position = this->gl_Position * scalar;
      out.uv = this->uv * scalar;
      return out;
    }
    VS_OUT operator+(const VS_OUT& frag) const {
      VS_OUT out;
      out.gl_Position = this->gl_Position + frag.gl_Position;
      out.uv = this->uv + frag.uv;
      return out;
    }
  } FS_IN;
  struct Shader {
    void VS(const Uniforms& uniforms, const VS_IN& vertex_in,
      VS_OUT& vertex_out, Vec4& gl_Position) const 
    {
      gl_Position = uniforms.transform * Vec4(vertex_in.xy, 0.0, 1.0);
      vertex_out.uv = vertex_in.uv;
    }
    void FS(const Uniforms& uniforms, const FS_IN& fragment_in, const Vec4& gl_FragCoord,
      FS_Outputs& fs_outs, bool& discard, double& gl_FragDepth) const {
      Vec4 tex_color = texture(uniforms.in_texture, fragment_in.uv);
      if (uniforms.in_mask != NULL && texture(uniforms.in_mask, fragment_in.uv).r < 0.99)
          discard = true;
      if (tex_color.a < 0.99)
        discard = true;
      if (discard)
        return;
      fs_outs[0] = Vec4(uniforms.color_mask, 1.0) * tex_color;
    }
  };
protected:
  typedef Pipeline<Uniforms, VS_IN, FS_IN, Shader> Pipeline_t;
  Pipeline_t pipeline;
  Shader       shader;
  Uniforms   uniforms;
  Vec2    vertices[4];
  Pipeline_t::IndexBuffer_t indices;
  SpriteOriginMode origin_mode;
public:
  void draw(const Texture* tex, const Vec2& pos, const Vec2& scale, const double& rot, const Vec3& color_mask, const Texture* src_mask = NULL) {
    Mat4x4 scaling(
      scale.x, 0.0, 0.0, 0.0,
      0.0, scale.y, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0);
    Mat4x4 translation(
      1.0, 0.0, 0.0, pos.x,
      0.0, 1.0, 0.0, pos.y,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0);
    Mat4x4 rotation(quat_to_mat3x3(Quat::rot_z(rot)));
    Mat4x4 model = translation * rotation * scaling;
    Mat4x4 projection = get_orthographic_matrix(0.0, 1.0, 0.0, 
      this->pipeline.get_render_target(0)->get_width(), 
      this->pipeline.get_render_target(0)->get_height(), 0.0);
    Pipeline_t::VertexBuffer_t vbuf;
    Vec2 texture_size = Vec2(tex->get_width(), tex->get_height());
    vbuf.resize(4);
    vbuf[0].xy = texture_size * vertices[0];
    vbuf[1].xy = texture_size * vertices[1];
    vbuf[2].xy = texture_size * vertices[2];
    vbuf[3].xy = texture_size * vertices[3];
    vbuf[0].uv = Vec2(0.0, 0.0);
    vbuf[1].uv = Vec2(1.0, 0.0);
    vbuf[2].uv = Vec2(1.0, 1.0);
    vbuf[3].uv = Vec2(0.0, 1.0);
    uniforms.transform = projection * model;
    uniforms.color_mask = color_mask;
    uniforms.in_texture = tex;
    uniforms.in_mask = src_mask;
    pipeline.set_depth_test_state(false);
    pipeline.draw(shader, vbuf, indices, uniforms);
    pipeline.set_depth_test_state(true);
  }
  void draw(const Texture* tex, int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, const Vec2& scale, const double& rot, const Vec3& color_mask, const Texture* src_mask = NULL) {
    Mat4x4 scaling(
      scale.x, 0.0, 0.0, 0.0,
      0.0, scale.y, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0);
    Mat4x4 translation(
      1.0, 0.0, 0.0, double(dst_x),
      0.0, 1.0, 0.0, double(dst_y),
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0);
    Mat4x4 rotation(quat_to_mat3x3(Quat::rot_z(rot)));
    Mat4x4 model = translation * rotation * scaling;
    Mat4x4 projection = get_orthographic_matrix(0.0, 1.0, 0.0,
      this->pipeline.get_render_target(0)->get_width(),
      this->pipeline.get_render_target(0)->get_height(), 0.0);
    Pipeline_t::VertexBuffer_t vbuf;
    Vec2 src_sprite_size = Vec2(src_w, src_h);
    vbuf.resize(4);
    vbuf[0].xy = src_sprite_size * vertices[0];
    vbuf[1].xy = src_sprite_size * vertices[1];
    vbuf[2].xy = src_sprite_size * vertices[2];
    vbuf[3].xy = src_sprite_size * vertices[3];
    double du = 1.0 / double(tex->get_width()), dv = 1.0 / double(tex->get_height());
    double lx = du * double(src_x), rx = du * (src_x + src_w);
    double by = dv * double(tex->get_height() - src_y - src_h), ty = dv * double(tex->get_height() - src_y);
    vbuf[0].uv = Vec2(lx, by);
    vbuf[1].uv = Vec2(rx, by);
    vbuf[2].uv = Vec2(rx, ty);
    vbuf[3].uv = Vec2(lx, ty);
    uniforms.transform = projection * model;
    uniforms.color_mask = color_mask;
    uniforms.in_texture = tex;
    uniforms.in_mask = src_mask;
    pipeline.set_depth_test_state(false);
    pipeline.draw(shader, vbuf, indices, uniforms);
    pipeline.set_depth_test_state(true);
  }
  void bind_render_target(Texture* target) { this->pipeline.bind_render_target(0, target); }
  void set_num_threads(int num_threads) { this->pipeline.set_num_threads(num_threads); }
  void set_sprite_origin_mode(SpriteOriginMode mode) { 
    this->origin_mode = mode; 
    if (this->origin_mode == SpriteOriginMode_Center) {
      vertices[0] = Vec2(-0.5, -0.5); vertices[1] = Vec2(+0.5, -0.5);
      vertices[2] = Vec2(+0.5, +0.5); vertices[3] = Vec2(-0.5, +0.5);
    }
    else if (this->origin_mode == SpriteOriginMode_BottomLeft) {
      vertices[0] = Vec2(+0.0, +0.0); vertices[1] = Vec2(+1.0, +0.0);
      vertices[2] = Vec2(+1.0, +1.0); vertices[3] = Vec2(+0.0, +1.0);
    }
    else if (this->origin_mode == SpriteOriginMode_BottomRight) {
      vertices[0] = Vec2(-1.0, +0.0); vertices[1] = Vec2(+0.0, +0.0);
      vertices[2] = Vec2(+0.0, +1.0); vertices[3] = Vec2(-1.0, +1.0);
    }
    else if (this->origin_mode == SpriteOriginMode_TopLeft){
      vertices[0] = Vec2(+0.0, -1.0); vertices[1] = Vec2(+1.0, -1.0);
      vertices[2] = Vec2(+1.0, +0.0); vertices[3] = Vec2(+0.0, +0.0);
    }
    else if (this->origin_mode == SpriteOriginMode_TopRight) {
      vertices[0] = Vec2(-1.0, -1.0); vertices[1] = Vec2(+0.0, -1.0);
      vertices[2] = Vec2(+0.0, +0.0); vertices[3] = Vec2(-1.0, +0.0);
    }
  }
  void clear_pipeline_cache() { this->pipeline.clear_cache(); }
  void clear_render_target(const Vec4& clear_color) { this->pipeline.clear_render_targets(clear_color); }

public:
  SpriteRenderer() {
    indices.resize(6);
    indices[0] = 0; indices[1] = 1; indices[2] = 3;
    indices[3] = 1; indices[4] = 2; indices[5] = 3;
    this->set_sprite_origin_mode(SpriteOriginMode_Center);
  }

};

extern SpriteRenderer sprite_renderer;

/**
Performs a texture bit-block transfer (blit) with enhanced functionality,
including support for scaling and rotation. However, this operation is 
slightly slower compared to the basic blit functions implemented in 
`sgl_texture.h`.
**/
void blit_texture(sgl::Texture* source, sgl::Texture* target,
  int src_x, int src_y, int src_w, int src_h, int dst_x, int dst_y, 
  const Vec2& scale, const double& rot, const Vec3& color_mask,
  const SpriteOriginMode origin_mode = SpriteOriginMode_TopLeft, 
  const Texture* src_mask = NULL);


}; /* namespace sgl */