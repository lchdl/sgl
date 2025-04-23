#version 430 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord;

uniform mat4x4 u_Model;
uniform mat4x4 u_View;
uniform mat4x4 u_Projection;
uniform float u_dz;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;

void main()
{
  mat4x4 Transform = u_Projection * u_View * u_Model;
  gl_Position = Transform * vec4(in_Position, 1.0);
  TexCoord = in_TexCoord;
  WorldNormal = (u_Model * vec4(in_Normal, 0.0)).xyz;
  WorldPosition = (u_Model * vec4(in_Position, 1.0)).xyz;
  gl_Position.z += u_dz;
}
