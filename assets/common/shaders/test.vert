#version 330 core
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
uniform mat4x4 Model;
uniform mat4x4 View;
uniform mat4x4 Projection;
out vec2 TexCoord;
void main()
{
	gl_Position = Projection * View * Model * vec4(inPosition, 1.0);
	TexCoord = inTexCoord;
}