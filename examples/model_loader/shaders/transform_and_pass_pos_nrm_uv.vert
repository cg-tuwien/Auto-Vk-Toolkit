#version 460

layout (location = 0) in vec4 inPosition; 
layout (location = 1) in vec2 inTexCoord;
layout (location = 2) in vec3 inNormal;

layout(push_constant) uniform PushConstants {
	mat4 modelMatrix;
	mat4 projViewMatrix;
} pushConstants;

out gl_PerVertex 
{
	vec3 positionWS;
	vec3 normalWS;
	vec2 texCoord;
	vec4 gl_Position;   
};

void main() {
	positionWS = pushConstants.modelMatrix * vec4(inPosition, 1.0);
    texCoord = inTexCoord;
	normalWS = mat3(pushConstants.modelMatrix) * inNormal;
    gl_Position = pushConstants.projViewMatrix * positionWS;
}
