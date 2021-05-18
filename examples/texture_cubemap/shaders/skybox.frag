// copied from https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/texturecubemap/skybox.frag
#version 450

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	outFragColor = texture(samplerCubeMap, inUVW);
}
