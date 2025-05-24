#version 330 core
layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec2 inTexCoord;
uniform mat4x4 Model;
uniform mat4x4 View;
uniform mat4x4 Projection;
uniform mat4x4 LightTransform;
out vec2 TexCoord;
out vec4 LightSpacePos;
void main()
{
  gl_Position = Projection * View * Model * vec4(inPosition, 1.0);
  TexCoord = inTexCoord;
  LightSpacePos = LightTransform * Model * vec4(inPosition, 1.0);
}