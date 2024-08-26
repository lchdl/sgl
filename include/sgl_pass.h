#pragma once

#include "sgl_pipeline.h"

namespace sgl {

/**

A `pass` is an object that describes a complete render operation
and stores all the resources used during rendering. All `pass`
objects & instances should inherit from `Pass` base class.

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

class Pass {
public:
  /* camera/eye settings */
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
public:
  /* utility functions */
  Mat4x4 get_view_matrix() const;
  Mat4x4 get_projection_matrix(int w, int h) const;
  /* default ctor & dtor */
  Pass();
  virtual ~Pass() {}
};

/**
TemplatePass:

A standard template for fully utilizing the programmable pipeline feature
of SGL. The implementation of this class can also serve as a tutorial.
**/
class TemplatePass : public Pass {
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
    void  operator*=(const double& scalar) {
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
  /* ... other member variables here ... */
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
BaseAnimator:

Simply draw a model (probably with animation) onto screen.
* Note that a model can consists of multiple meshes, so this class also 
  wraps up multiple draw calls to fully render a model, each draw call
  only renders a single mesh.
**/
class BaseAnimator : public Pass
{
public:
  struct Uniforms {
    Mat4x4 world;
    Mat4x4 view;
    Mat4x4 projection;
    /* texture objects */
    const Texture *in_textures[MAX_TEXTURES_PER_SHADING_UNIT];
    /* final bone transformations */
    Mat4x4 bone_matrices[MAX_NODES_PER_MODEL];
  };
  struct VS_IN : public IVertex {
    Vec3 p; /* vertex position (in model local space) */
    Vec3 n; /* vertex normal (in model local space)*/
    Vec2 t; /* vertex texture coordinate */
    /* for skeletal animations */
    IVec4 bone_IDs; /* bones up to 4 */
    Vec4  bone_weights;
  };
  typedef struct VS_OUT : public IFragment {
    Vec3 wp; /* world position */
    Vec3 wn; /* world normal */
    Vec2 t;  /* texture coordinates */

    /* The `Fragment` class need define how two fragments should be interpolated.  */
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
  void                 load_model(const std::string& file);
  void        set_model_transform(const Mat4x4& transform);
  PipelineDrawMode  get_draw_mode() const { return this->pipeline.get_draw_mode(); }
  void              set_draw_mode(PipelineDrawMode draw_mode) { this->pipeline.set_draw_mode(draw_mode); }
  bool get_backface_culling_state() const { return this->pipeline.get_backface_culling_state(); }
  void set_backface_culling_state(bool state) { this->pipeline.set_backface_culling_state(state); }
  void            set_num_threads(int num_threads) { this->pipeline.set_num_threads(num_threads); }
  void             play_animation(const std::string& anim_name, const double& play_time) { this->anim_name = anim_name; this->play_time = play_time; }
  double     query_last_draw_time() const { return this->last_draw_time; }
  void         set_render_targets(Texture* color, Texture* depth, Texture* normal) { 
    this->pipeline.bind_render_target(0, color);
    this->pipeline.bind_render_target(1, depth);
    this->pipeline.bind_render_target(2, normal);
  }
  void       clear_pipeline_cache() { this->pipeline.clear_cache(); }
  void       clear_render_targets(const Vec4& clear_color) { this->pipeline.clear_render_targets(clear_color); }

public:
  BaseAnimator();
  virtual ~BaseAnimator() {}

protected:
  VS_IN convert_from_mesh_vertex(const Vertex_pnt_nm_bone& v) const;

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

};


/**
SpriteRenderer:

Draw a 2D sprite onto screen.
**/
class SpriteRenderer : public Pass {
public:
  struct Uniforms {
    Vec3 color_mask;
    Mat4x4 transform;
    const Texture* in_texture;
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
      if (tex_color.a < 0.99) {
        discard = true;
        return;
      }
      fs_outs[0] = Vec4(uniforms.color_mask, 1.0) * tex_color;
    }
  };
protected:
  typedef Pipeline<Uniforms, VS_IN, FS_IN, Shader> Pipeline_t;
  Pipeline_t pipeline;
  Shader       shader;
  Uniforms   uniforms;
  Pipeline_t::VertexBuffer_t vertices;
  Pipeline_t::IndexBuffer_t   indices;
public:
  void draw(const Texture* tex, const Vec2& pos, const Vec2& scale, const double& rot, const Vec3& color_mask) {
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
      this->pipeline.get_render_target(0)->w, 
      this->pipeline.get_render_target(0)->h, 0.0);
    vertices[0].xy = Vec2(-0.5*tex->w, -0.5*tex->h);
    vertices[1].xy = Vec2(+0.5*tex->w, -0.5*tex->h);
    vertices[2].xy = Vec2(+0.5*tex->w, +0.5*tex->h);
    vertices[3].xy = Vec2(-0.5*tex->w, +0.5*tex->h);
    uniforms.transform = projection * model;
    uniforms.color_mask = color_mask;
    uniforms.in_texture = tex;
    pipeline.set_depth_test_state(false);
    pipeline.draw(shader, vertices, indices, uniforms);
    pipeline.set_depth_test_state(true);
  }
  void set_render_target(Texture* target) { this->pipeline.bind_render_target(0, target); }
  void set_num_threads(int num_threads) { this->pipeline.set_num_threads(num_threads); }
  void clear_pipeline_cache() { this->pipeline.clear_cache(); }
  void clear_render_target(const Vec4& clear_color) { this->pipeline.clear_render_targets(clear_color); }


public:
  SpriteRenderer() {
    vertices.resize(4);
    indices.resize(6);
    vertices[0].uv = Vec2(0.0, 0.0);
    vertices[1].uv = Vec2(1.0, 0.0);
    vertices[2].uv = Vec2(1.0, 1.0);
    vertices[3].uv = Vec2(0.0, 1.0);
    indices[0] = 0; indices[1] = 1; indices[2] = 3;
    indices[3] = 1; indices[4] = 2; indices[5] = 3;
  }

};







}; /* namespace sgl */