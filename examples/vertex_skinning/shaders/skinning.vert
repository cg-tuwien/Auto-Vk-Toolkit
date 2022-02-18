#version 460
#extension GL_EXT_debug_printf : enable
#extension GL_GOOGLE_include_directive : require
#include "dual_quaternion.glsl"

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBiTangent;
layout (location = 5) in vec4 inBoneWeights;
layout (location = 6) in uvec4 inBoneIndices;
layout (location = 7) in vec3 inCenterOfRotation;

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

layout(set = 1, binding = 0) readonly buffer BoneMatricesBuffer {
	mat4 mat[];
} boneMatrices;

layout(set = 2, binding = 0) readonly buffer DualQuaternionsBuffer {
	DualQuaternion mDqs[];
} dqb;

layout (location = 0) out vec3 positionWS;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec2 texCoord;
layout (location = 3) flat out int materialIndex;

/*void debugMsg(int vertexIndex, int lineNr) {
	if(gl_VertexIndex == vertexIndex) {
		debugPrintfEXT("gl_VertexIndex = %d, lineNr = %d", gl_VertexIndex, lineNr);
	}
}*/

mat4 skin() {
	mat4 skinning_matrix = mat4(0.0);
	if (pushConstants.mSkinningMode == 1) {
		// LBS
		skinning_matrix = boneMatrices.mat[inBoneIndices.x] * inBoneWeights.x
		+ boneMatrices.mat[inBoneIndices.y] * inBoneWeights.y
		+ boneMatrices.mat[inBoneIndices.z] * inBoneWeights.z
		+ boneMatrices.mat[inBoneIndices.w] * inBoneWeights.w;
	} else if (pushConstants.mSkinningMode == 2) {
		// DQS
		DualQuaternion dqResult = DualQuaternion(vec4(0,0,0,0), vec4(0));
		for (int i = 0; i < 4; ++i) {
			dqResult = dualquat_add(dqResult, dualquat_mult(inBoneWeights[i], dqb.mDqs[inBoneIndices[i]]));
		}

		DualQuaternion c = dualquat_normalize(dqResult);
		vec4 translation = vec4(dualquat_getTranslation(c), 1);
		skinning_matrix = mat4(quat_toRotationMatrix(c.real));
		skinning_matrix[3] = translation;
		//debugMsg(77);
	} else if (pushConstants.mSkinningMode == 3) {
		// COR
		vec4 quatRotation = vec4(0);
		mat4 lbs = mat4(0);

		for (int i = 0; i < 4; ++i) {
			quatRotation = quat_add_oriented(quatRotation, inBoneWeights[i] * dqb.mDqs[inBoneIndices[i]].real);
			lbs += inBoneWeights[i] * boneMatrices.mat[inBoneIndices[i]];
		}

		quatRotation = normalize(quatRotation);
		mat3 quatRotationMatrix = quat_toRotationMatrix(quatRotation);

		vec4 translation = vec4((lbs*vec4(inCenterOfRotation, 1.0) - vec4(quatRotationMatrix*inCenterOfRotation, 0.0)).xyz, 1.0);

		skinning_matrix = mat4(quatRotationMatrix);
		skinning_matrix[3] = translation;
	}
	return skinning_matrix;
}

void main() {
	// debugPrintfEXT("debug text vong shader wuhu");

	mat4 totalMatrix;
	if (pushConstants.mSkinningMode == 0) {
		// Static meshes should not be multiplied by an empty skinning_matrix, or they get 0 everywhere.
		totalMatrix = pushConstants.mModelMatrix;
	} else {
		totalMatrix = pushConstants.mModelMatrix * skin();
	}

	vec4 posWS = totalMatrix * vec4(inPosition.xyz, 1.0);
	positionWS = posWS.xyz;
	gl_Position = ct.mViewProjMatrix * posWS;
	//normalWS = mat3(totalMatrix) * inNormal;
	normalWS = (transpose(inverse(totalMatrix)) * vec4(inNormal, 1.0)).xyz;
	texCoord = inTexCoord;
	materialIndex = pushConstants.mMaterialIndex;
}