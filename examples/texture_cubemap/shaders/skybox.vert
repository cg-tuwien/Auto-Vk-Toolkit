// adapted from https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/texturecubemap/skybox.vert
#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec3 outUVW;

void main() 
{
	// the skybox vertices are already in a left-handed coordinate system,
	// so we can pass them to the fragment shader as-is for texture coordinate lookup
	outUVW = inPos;

	// the model matrix of the skybox takes care of transforming its vertices to right-handed world coordinates
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}
