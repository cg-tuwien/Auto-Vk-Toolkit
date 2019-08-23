#version 460

layout (location = 0) in vec3 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

layout(push_constant) uniform PushConstants {
	mat4 modelMatrix;
	mat4 projViewMatrix;
} pushConstants;

layout (location = 0) out vec3 positionWS;
layout (location = 1) out vec3 normalWS;
layout (location = 2) out vec2 texCoord;

void main() {
	vec4 posWS = pushConstants.modelMatrix * vec4(inPosition.xyz, 1.0);
	positionWS = posWS.xyz;
    texCoord = inTexCoord;
	normalWS = mat3(pushConstants.modelMatrix) * inNormal;
    gl_Position = pushConstants.projViewMatrix * posWS;
}
