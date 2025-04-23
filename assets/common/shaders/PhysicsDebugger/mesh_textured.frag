#version 430 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragNormal;

uniform sampler2D tex0; /* diffuse */

in vec3 WorldPosition;
in vec3 WorldNormal;
in vec2 TexCoord;

void main()
{
  vec3 textured = texture(tex0, TexCoord).xyz;
  float falloff = (dot(WorldNormal, vec3(0.0, 1.0, 0.0)) + 1.0) * 0.5;
  FragColor = vec4(textured * falloff, 1.0);
  FragNormal = vec4((WorldNormal + 1.0) * 0.5, 1.0);
}
