#version 430 core
layout (location = 0) in vec3 in_Position;
layout (location = 1) in vec3 in_Normal;
layout (location = 2) in vec2 in_TexCoord;
layout (location = 3) in vec3 in_Tangent;
layout (location = 4) in vec3 in_BiTangent;
layout (location = 5) in ivec4 in_BoneIDs;
layout (location = 6) in vec4 in_BoneWeights;

layout(binding = 0, std430) readonly buffer ssbo1 { 
  mat4x4 in_ModelMatrices[]; 
};
layout(binding = 1, std430) readonly buffer ssbo2 { 
  mat4x4 in_BoneMatrices[]; 
};

const int MAX_NODES_PER_MODEL = 256;
const int MAX_BONES_INFLUENCE_PER_VERTEX = 4;

uniform mat4x4 u_View;
uniform mat4x4 u_Projection;

out vec3 WorldPosition;
out vec3 WorldNormal;
out vec2 TexCoord;

void main()
{
  mat4x4 in_Model = in_ModelMatrices[gl_InstanceID];
  mat4x4 Transform = u_Projection * u_View * in_Model;

  if (in_BoneIDs.x < 0) {
    /* Vertex does not belong to any bone */
    gl_Position = Transform * vec4(in_Position, 1.0);
    TexCoord = in_TexCoord;
    WorldNormal = normalize((in_Model * vec4(in_Normal, 0.0)).xyz);
    WorldPosition = (in_Model * vec4(in_Position, 1.0)).xyz;
  }
  else {
    /* 
    Vertex is controlled by at least one bone.
    So now we calculate:
         p_final = sum( w[i] * m[i] * p, for i in [0,1,2,3] ), 
    where `p` is the vertex position in local model space (T-pose),
    `m[i]` is the i-th final bone transformation matrix, `w[i]` is 
    the i-th bone influence weight to the vertex. to make computat-
    ion a little bit faster, we calculate
                  T = w[i] * m[i], for i in [0,1,2,3], 
    then multiply `T` with `p` to get `p_final`.
    */
    mat4x4 BoneTransform = mat4x4(0.0);
    for (int i_bone = 0; i_bone < MAX_BONES_INFLUENCE_PER_VERTEX; i_bone++) {
      int bone_id = in_BoneIDs[i_bone];
      /* bone_id can be negative, which indicates that the
       * corresponding and subsequent slots are unused. */
      if (bone_id < 0) break;
      float bone_weight = in_BoneWeights[i_bone];
      mat4x4 BoneMatrix = in_BoneMatrices[MAX_NODES_PER_MODEL * gl_InstanceID + bone_id];
      BoneTransform += bone_weight * BoneMatrix;
    }
    vec4 p0 = BoneTransform * vec4(in_Position, 1.0);
    vec4 n0 = BoneTransform * vec4(in_Normal, 0.0);
    /* apply final matrix to vertex position */
    gl_Position = Transform * p0;
    TexCoord = in_TexCoord;
    WorldNormal = normalize((in_Model * n0).xyz);
    WorldPosition = (in_Model * p0).xyz;
  }
}
