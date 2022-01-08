#version 460

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in vec4 inBoneWeights;
layout (location = 6) in uvec4 inBoneIndices;

layout(push_constant) uniform PushConstants {
	mat4 mModelMatrix;
	vec4 mCamPos;
	int mMaterialIndex;
} pushConstants;

layout(set = 0, binding = 0) uniform CameraTransform
{
	mat4 mViewProjMatrix;
} ct;

layout(set = 1, binding = 0) readonly buffer BoneMatricesBuffer{
	mat4 mat[];
} boneMatrices;

layout (location = 0) out vec3 positionWS;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec2 texCoord;
layout (location = 3) flat out int materialIndex;

void main() {
	mat4 skinning_matrix = boneMatrices.mat[inBoneIndices[0]] * inBoneWeights[0]
						 + boneMatrices.mat[inBoneIndices[1]] * inBoneWeights[1]
				         + boneMatrices.mat[inBoneIndices[2]] * inBoneWeights[2]
				         + boneMatrices.mat[inBoneIndices[3]] * inBoneWeights[3];
	mat4 totalMatrix = pushConstants.mModelMatrix * skinning_matrix;
	vec4 posWS = totalMatrix * vec4(inPosition.xyz, 1.0);
	positionWS = posWS.xyz;
    gl_Position = ct.mViewProjMatrix * posWS;
	//normalWS = mat3(totalMatrix) * inNormal;
	normalWS = (transpose(inverse(totalMatrix)) * vec4(inNormal, 1.0)).xyz;
	texCoord = inTexCoord;
	materialIndex = pushConstants.mMaterialIndex;
}