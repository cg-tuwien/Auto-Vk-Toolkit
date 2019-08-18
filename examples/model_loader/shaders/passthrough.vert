#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConsts {
	mat4 modelViewProjMat;
} pushConsts;

void main() {
    gl_Position = pushConsts.modelViewProjMat * vec4(inPosition, 1.0);
    fragColor = inColor;
}
