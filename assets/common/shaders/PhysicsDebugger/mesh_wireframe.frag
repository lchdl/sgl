#version 430 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragNormal;

uniform vec3 color;

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;

void main()
{
  FragColor = vec4(color, 1.0);
  FragNormal = vec4((WorldNormal + 1.0) * 0.5, 1.0);
}
