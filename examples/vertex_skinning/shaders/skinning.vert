#version 460
#extension GL_GOOGLE_include_directive : require
#include "dual_quaternion.glsl"

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
	int mSkinningMode; // 0 = LBS, 1 = DQS, 2 = CORS
} pushConstants;

layout(set = 0, binding = 0) uniform CameraTransform
{
	mat4 mViewProjMatrix;
} ct;

layout(set = 1, binding = 0) readonly buffer BoneMatricesBuffer{
	mat4 mat[];
} boneMatrices;

layout(set = 2, binding = 0) readonly buffer DualQuaternionsBuffer {
	DualQuaternion mDqs[];
} dqb;

layout (location = 0) out vec3 positionWS;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec2 texCoord;
layout (location = 3) flat out int materialIndex;

void main() {
	mat4 skinning_matrix = mat4(1.0);
	if (pushConstants.mSkinningMode == 0) {
		// LBS
		skinning_matrix = boneMatrices.mat[inBoneIndices.x] * inBoneWeights.x
						+ boneMatrices.mat[inBoneIndices.y] * inBoneWeights.y
						+ boneMatrices.mat[inBoneIndices.z] * inBoneWeights.z
						+ boneMatrices.mat[inBoneIndices.w] * inBoneWeights.w;
	} else if (pushConstants.mSkinningMode == 1) {
		// DQS
		DualQuaternion dq0 = dualquat_mult(inBoneWeights.x, dqb.mDqs[inBoneIndices.x]);
		DualQuaternion dq1 = dualquat_mult(inBoneWeights.y, dqb.mDqs[inBoneIndices.y]);
		DualQuaternion dq2 = dualquat_mult(inBoneWeights.z, dqb.mDqs[inBoneIndices.z]);
		DualQuaternion dq3 = dualquat_mult(inBoneWeights.w, dqb.mDqs[inBoneIndices.w]);
		vec4 rot_quat = quat_add_oriented(dq0.r, dq1.r);
		rot_quat = quat_add_oriented(rot_quat, dq2.r);
		rot_quat = quat_add_oriented(rot_quat, dq3.r);
		vec4 trans_quat = dq0.d + dq1.d + dq2.d + dq3.d;
		float mag = length(rot_quat);
		rot_quat = rot_quat / mag;
		trans_quat = trans_quat / mag;
		DualQuaternion dqw = DualQuaternion(rot_quat, trans_quat);
		skinning_matrix = dualquat_to_matrix(dqw);
	}

	mat4 totalMatrix = pushConstants.mModelMatrix * skinning_matrix;
	vec4 posWS = totalMatrix * vec4(inPosition.xyz, 1.0);
	positionWS = posWS.xyz;
    gl_Position = ct.mViewProjMatrix * posWS;
	//normalWS = mat3(totalMatrix) * inNormal;
	normalWS = (transpose(inverse(totalMatrix)) * vec4(inNormal, 1.0)).xyz;
	texCoord = inTexCoord;
	materialIndex = pushConstants.mMaterialIndex;
}