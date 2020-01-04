#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

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

layout(set = 0, binding = 1) buffer Material 
{
	MaterialGpuData materials[];
} matSsbo;

layout(set = 0, binding = 2) uniform usamplerBuffer indexBuffers[];
layout(set = 0, binding = 3) uniform samplerBuffer vertexDataBuffers[];

layout(set = 2, binding = 0) uniform accelerationStructureNV topLevelAS;

layout(location = 0) rayPayloadInNV vec3 hitValue;
hitAttributeNV vec3 attribs;
layout(location = 2) rayPayloadNV float secondaryRayHitValue;

layout(push_constant) uniform PushConstants {
	mat4 viewMatrix;
    vec4 lightDir;
} pushConstants;

void main()
{
    const vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);
	const int instanceIndex = nonuniformEXT(gl_InstanceCustomIndexNV);
	const ivec3 indices = ivec3(texelFetch(indexBuffers[instanceIndex], gl_PrimitiveID).rgb);
	const vec2 uv0 = texelFetch(vertexDataBuffers[instanceIndex], indices.x).rg;
	const vec2 uv1 = texelFetch(vertexDataBuffers[instanceIndex], indices.y).rg;
	const vec2 uv2 = texelFetch(vertexDataBuffers[instanceIndex], indices.z).rg;
	const vec2 uv = (barycentrics.x * uv0 + barycentrics.y * uv1 + barycentrics.z * uv2);

    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
    vec3 direction = normalize(pushConstants.lightDir.xyz);
    uint rayFlags = gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV;
    uint cullMask = 0xff;
    float tmin = 0.001;
    float tmax = 100.0;

    traceNV(topLevelAS, rayFlags, cullMask, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, origin, tmin, direction, tmax, 2 /*payload location*/);

//	 gl_InstanceCustomIndex = VkGeometryInstance::instanceId
//    hitValue = (barycentrics * 0.5 + vec3(0.5, 0.5, 0.5))
//		* uniformBuffers[nonuniformEXT(gl_InstanceCustomIndexNV)].color.rgb
//		* ( secondaryRayHitValue < tmax ? 0.25 : 1.0 );
//	hitValue = barycentrics * 0.5 + vec3(0.5, 0.5, 0.5);
	hitValue = vec3(clamp(uv,0,1), 0) * (secondaryRayHitValue < tmax ? 0.25 : 1.0);
}