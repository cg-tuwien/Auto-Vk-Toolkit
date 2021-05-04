#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 4, binding = 4) uniform sampler2D textures[];

struct MaterialGpuData
{
	vec4 mDiffuseReflectivity;
	vec4 mAmbientReflectivity;
	vec4 mSpecularReflectivity;
	vec4 mEmissiveColor;
	vec4 mTransparentColor;
	vec4 mReflectiveColor;
	vec4 mAlbedo;

	float mOpacity;
	float mBumpScaling;
	float mShininess;
	float mShininessStrength;
	
	float mRefractionIndex;
	float mReflectivity;
	float mMetallic;
	float mSmoothness;
	
	float mSheen;
	float mThickness;
	float mRoughness;
	float mAnisotropy;
	
	vec4 mAnisotropyRotation;
	vec4 mCustomData;
	
	int mDiffuseTexIndex;
	int mSpecularTexIndex;
	int mAmbientTexIndex;
	int mEmissiveTexIndex;
	int mHeightTexIndex;
	int mNormalsTexIndex;
	int mShininessTexIndex;
	int mOpacityTexIndex;
	int mDisplacementTexIndex;
	int mReflectionTexIndex;
	int mLightmapTexIndex;
	int mExtraTexIndex;
	
	vec4 mDiffuseTexOffsetTiling;
	vec4 mSpecularTexOffsetTiling;
	vec4 mAmbientTexOffsetTiling;
	vec4 mEmissiveTexOffsetTiling;
	vec4 mHeightTexOffsetTiling;
	vec4 mNormalsTexOffsetTiling;
	vec4 mShininessTexOffsetTiling;
	vec4 mOpacityTexOffsetTiling;
	vec4 mDisplacementTexOffsetTiling;
	vec4 mReflectionTexOffsetTiling;
	vec4 mLightmapTexOffsetTiling;
	vec4 mExtraTexOffsetTiling;
};

layout(set = 7, binding = 9) buffer Material 
{
	MaterialGpuData materials[];
} matSsbo;

layout (location = 0) in vec3 positionWS;
layout (location = 1) in vec3 normalWS;
layout (location = 2) in vec2 texCoord;
layout (location = 3) flat in int materialIndex;

layout (location = 0) out vec4 fs_out;

void main() 
{
	int matIndex = materialIndex;

	int diffuseTexIndex = matSsbo.materials[matIndex].mDiffuseTexIndex;
	vec2 diffTexTiling  = matSsbo.materials[matIndex].mDiffuseTexOffsetTiling.zw;
	vec2 diffTexOffset  = matSsbo.materials[matIndex].mDiffuseTexOffsetTiling.xy;
    vec3 color = texture(textures[diffuseTexIndex], texCoord * diffTexTiling + diffTexOffset).rgb;
	
	float ambient = 0.1;
	vec3 diffuse = matSsbo.materials[matIndex].mDiffuseReflectivity.rgb;
	vec3 toLight = normalize(vec3(1.0, 1.0, 0.5));
	vec3 illum = vec3(ambient) + diffuse * max(0.0, dot(normalize(normalWS), toLight));
	color *= illum;
	
	fs_out = vec4(color, 1.0);
}