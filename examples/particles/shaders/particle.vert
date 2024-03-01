#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PART_CNT 3

struct Particle {
    vec3 pos;
	vec3 vel;
	float size;
	float age;
};

layout(set = 0, binding = 0) uniform ParticleBuffer
{
    Particle[PART_CNT] p;
} particleBuffer;

layout(location = 0) out vec3 fragColor;

vec2 positions[6] = vec2[](
    vec2(-0.5, -0.5),
    vec2(0.5, -0.5),
    vec2(-0.5, 0.5),

    vec2(0.5, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    uint id = gl_VertexIndex / 6;
    Particle p = particleBuffer.p[id];
    gl_Position = vec4(positions[gl_VertexIndex % 6]*p.size*p.age + p.pos.xy /*+ vec2(id/3.,0.)*/, 0.0, 1.0);
    //gl_Position += vec4(p.vel * (p.age + p.pos.z), 0.);
    fragColor = colors[gl_VertexIndex % 3];
}