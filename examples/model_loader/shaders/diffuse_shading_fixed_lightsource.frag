#version 460
#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout(set = 1, binding = 0) uniform Material
{
	vec4 m_diffuse_reflectivity;
	vec4 m_specular_reflectivity;
	vec4 m_ambient_reflectivity;
	vec4 m_emissive_color;
	vec4 m_transparent_color;
	vec4 m_albedo;
	
	float m_opacity;
	float m_shininess;
	float m_shininess_strength;
	float m_refraction_index;
	
	float m_reflectivity;
	float m_metallic;
	float m_smoothness;
	float m_sheen;
	
	float m_thickness;
	float m_roughness;
	float m_anisotropy;
	float m_extra_param;
	
	vec4 m_anisotropy_rotation;
	
	int m_diffuse_tex_index;
	int m_specular_tex_index;
	int m_ambient_tex_index;
	int m_emissive_tex_index;
	int m_height_tex_index;
	int m_normals_tex_index;
	int m_shininess_tex_index;
	int m_opacity_tex_index;
	int m_displacement_tex_index;
	int m_reflection_tex_index;
	int m_lightmap_tex_index;
	int m_extra_tex_index;
	
	vec4 m_diffuse_tex_offset_tiling;
	vec4 m_specular_tex_offset_tiling;
	vec4 m_ambient_tex_offset_tiling;
	vec4 m_emissive_tex_offset_tiling;
	vec4 m_height_tex_offset_tiling;
	vec4 m_normals_tex_offset_tiling;
	vec4 m_shininess_tex_offset_tiling;
	vec4 m_opacity_tex_offset_tiling;
	vec4 m_displacement_tex_offset_tiling;
	vec4 m_reflection_tex_offset_tiling;
	vec4 m_lightmap_tex_offset_tiling;
	vec4 m_extra_tex_offset_tiling;
} material;

layout (location = 0) in vec3 positionWS;
layout (location = 1) in vec3 normalWS;
layout (location = 2) in vec2 texCoord;

layout (location = 0) out vec4 fs_out;

void main() 
{
    vec3 color = texture(textures[material.m_diffuse_tex_index], texCoord).rgb;
	color *= material.m_diffuse_reflectivity.rgb;
	vec3 toLight = vec3(0.0, 1.0, 0.0);
	color *= max(0.0, dot(normalize(normalWS), toLight));
	fs_out = vec4(color, 1.0);
}