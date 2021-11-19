#version 460

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

layout(push_constant) uniform PushConstants {
	mat4 mModelMatrix;
	int mMaterialIndex;
} pushConstants;

layout(set = 0, binding = 1) uniform CameraTransform
{
	mat4 mViewProjMatrix;
} ubo;

layout (location = 0) out vec3 positionWS;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec2 texCoord;
layout (location = 3) flat out int materialIndex;

void main() {
	vec4 posWS = pushConstants.mModelMatrix * vec4(inPosition.xyz, 1.0);
	positionWS = posWS.xyz;
    texCoord = inTexCoord;
	normalWS = mat3(pushConstants.mModelMatrix) * inNormal;
	materialIndex = pushConstants.mMaterialIndex;
    gl_Position = ubo.mViewProjMatrix * posWS;
}

