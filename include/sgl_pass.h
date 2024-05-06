#pragma once

#include "sgl_pipeline.h"

namespace sgl {

/**

A `pass` is an object that describes a complete render operation
and stores all the resources used during rendering. All `pass`
objects & instances should inherit from `Pass` base class. All
child classes that inherit from class `Pass` must implement their
own version of "virtual void run(Pipeline& ppl)" function, as
this function describes a complete process of drawing something
onto screen.

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

struct Pass {
	/* output texture buffers (write only) */
	Texture* color_texture;
	Texture* depth_texture;
	/* uniform variables */
	Uniforms uniforms;
	/* vertex & fragment shaders */
	VS_func_t VS;
	FS_func_t FS;
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
	Mat4x4 get_projection_matrix() const;
	/* run the whole pass (rendering) */
	virtual void run(Pipeline& ppl) = 0;
	/* default ctor & dtor */
	Pass();
	virtual ~Pass() {}
};

/**
Simply draw a model (probably with animation) onto screen.
* Note that a model can consists of multiple meshes, so this class also 
  wraps up multiple draw calls to fully render a model, each draw call
  only renders a single mesh.
**/
struct ModelPass : public Pass {
public:
	Model* model; /* a pointer to model object that is being drawn */
	std::string anim_name; /* name of the current animation being played */
	double time; /* time value for controlling the skeletal animation (in sec.) */

public:
	virtual void run(Pipeline& ppl);

	ModelPass() { model = NULL; time = 0.0; }
	virtual ~ModelPass() {}
};

}; /* namespace sgl */