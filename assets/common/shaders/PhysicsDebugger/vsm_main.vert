#version 330 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord;
layout (location = 3) in vec3 in_Tangent;
layout (location = 4) in vec3 in_BiTangent;
layout (location = 5) in ivec4 in_BoneIDs;
layout (location = 6) in vec4 in_BoneWeights;
uniform mat4x4 Model;
uniform mat4x4 View;
uniform mat4x4 Projection;
uniform mat4x4 LightTransform;
out vec2 TexCoord;
out vec4 LightSpacePos;
out vec3 WorldNormal;
void main()
{
  gl_Position = Projection * View * Model * vec4(in_Position, 1.0);
  TexCoord = in_TexCoord;
  LightSpacePos = LightTransform * Model * vec4(in_Position, 1.0);
  WorldNormal = normalize((Model * vec4(in_Normal, 0.0)).xyz);
}