#include "sgl_pass.h"

namespace sgl {

Mat4x4
Pass::get_view_matrix() const {
	Vec3 front = normalize(eye.position - eye.look_at);
	Vec3 left = normalize(cross(eye.up_dir, front));
	Vec3 up = normalize(cross(front, left));
	Vec3 &F = front, &L = left, &U = up;
	const double &ex = eye.position.x, &ey = eye.position.y, &ez = eye.position.z;
	Mat4x4 rotation(L.x, L.y, L.z, 0.0, U.x, U.y, U.z, 0.0, F.x, F.y, F.z, 0.0,
		0.0, 0.0, 0.0, 1.0);
	Mat4x4 translation(1.0, 0.0, 0.0, -ex, 0.0, 1.0, 0.0, -ey, 0.0, 0.0, 1.0, -ez,
		0.0, 0.0, 0.0, 1.0);
	Mat4x4 view_matrix = mul(rotation, translation);
	return view_matrix;
}

Mat4x4
Pass::get_projection_matrix() const {
	Mat4x4 projection_matrix;
	if (eye.perspective.enabled) {
		double aspect_ratio = double(color_texture->w) / double(color_texture->h);
		double inv_aspect = double(1.0) / aspect_ratio;
		double near = eye.perspective.near;
		double far = eye.perspective.far;
		double field_of_view = eye.perspective.field_of_view;
		double left = -tan(field_of_view / double(2.0)) * near;
		double right = -left;
		double top = inv_aspect * right;
		double bottom = -top;
		projection_matrix =
			Mat4x4(2 * near / (right - left), 0, (right + left) / (right - left), 0,
				0, 2 * near / (top - bottom), (top + bottom) / (top - bottom), 0,
				0, 0, -(far + near) / (far - near),
				-2 * far * near / (far - near), 0, 0, -1.0, 0);
		return projection_matrix;
	}
	else {
		/* orthographic not implemented now */
		return projection_matrix;
	}
}

Pass::Pass()
{
	color_texture = NULL;
	depth_texture = NULL;
	VS = NULL;
	FS = NULL;
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
}

void
ModelPass::run(Pipeline& ppl) {
	if (this->model == NULL) return;

	ppl.set_shaders(this->VS, this->FS);
	ppl.set_render_targets(this->color_texture, this->depth_texture);
	ppl.clear_render_targets(this->color_texture, this->depth_texture, Vec4(0.5, 0.5, 0.5, 1.0));

	/* setup internal variables (gl_*) */
	if (this->eye.perspective.enabled) {
		uniforms.gl_DepthRange.x = this->eye.perspective.near;
		uniforms.gl_DepthRange.y = this->eye.perspective.far;
		uniforms.gl_DepthRange.z = uniforms.gl_DepthRange.y - uniforms.gl_DepthRange.x;
	}
	else {
		/* orthographic view not implemented. */
	}
	uniforms.model = this->model->get_model_transform();
	uniforms.view = this->get_view_matrix();
	uniforms.projection = this->get_projection_matrix();

	/* Rendering all the mesh parts in model */
	const std::vector<Mesh>& mesh_data = model->get_mesh_data();
	const std::vector<Material>& materials = model->get_materials();

	for (uint32_t i_mesh = 0; i_mesh < mesh_data.size(); i_mesh++) {
		const std::vector<Vertex>& vertices = mesh_data[i_mesh].vertices;
		const std::vector<int32_t>& indices = mesh_data[i_mesh].indices;
		const int32_t mat_id = mesh_data[i_mesh].mat_id;
		const Mesh& mesh = mesh_data[i_mesh];

		/* calculate bone tranformation matrices and update to uniforms */
		this->model->update_skeletal_animation_for_mesh(mesh, this->anim_name, this->time, uniforms);
		/* Setting up mesh materials. */
		uniforms.in_textures[0] = &materials[mat_id].diffuse_texture; /* diffuse texture */
		/* Launch the pipeline to render the triangles */
		ppl.draw(vertices, indices, uniforms);
	}
}

}; /* namespace sgl */
