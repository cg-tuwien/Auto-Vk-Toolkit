#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
	uint trianglePart;
} pushConstants;

vec2 positions[9] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.0, 0.0),
    vec2(-0.5, 0.5),
    vec2(0.0, 0.0),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5),
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(0.0, 0.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex + pushConstants.trianglePart * 3], 0.0, 1.0);
    fragColor = vec3(gl_Position.xy*2, max(0.5, gl_Position.x + gl_Position.y) );
}
