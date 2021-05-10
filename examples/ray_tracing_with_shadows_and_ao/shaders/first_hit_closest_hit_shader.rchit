#version 460
#extension GL_EXT_ray_tracing : require
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
} materialsBuffer;

layout(set = 0, binding = 2) uniform usamplerBuffer indexBuffers[];
layout(set = 0, binding = 3) uniform samplerBuffer texCoordsBuffers[];
layout(set = 0, binding = 4) uniform samplerBuffer normalsBuffers[];

layout(set = 2, binding = 0) uniform accelerationStructureEXT topLevelAS;

// Ray payload to be sent back to the ray generation shader (Hence rayPayloadInEXT, not rayPayloadEXT):
layout(location = 0) rayPayloadInEXT vec3 hitValue;

// Outgoing payload which is to be set by other shaders and evaluated here (hence rayPayloadEXT, not rayPayloadInEXT):
layout(location = 1) rayPayloadEXT vec3 shadowPayload;

// Outgoing payload which is to be set by other shaders and evaluated here (hence rayPayloadEXT, not rayPayloadInEXT):
layout(location = 2) rayPayloadEXT float aoPayload;

// Receive barycentric coordinates from the geometry hit:
hitAttributeEXT vec3 hitAttribs;

layout(push_constant) uniform PushConstants {
    vec4  mAmbientLight;
    vec4  mLightDir;
    mat4  mCameraTransform;
    float mCameraHalfFovAngle;
	float _padding;
    bool  mEnableShadows;
	float mShadowsFactor;
	vec4  mShadowsColor;
    bool  mEnableAmbientOcclusion;
	float mAmbientOcclusionMinDist;
	float mAmbientOcclusionMaxDist;
	float mAmbientOcclusionFactor;
	vec4  mAmbientOcclusionColor;
} pushConstants;

vec4 sample_from_diffuse_texture(int matIndex, vec2 uv)
{
	int texIndex = materialsBuffer.materials[matIndex].mDiffuseTexIndex;
	vec4 offsetTiling = materialsBuffer.materials[matIndex].mDiffuseTexOffsetTiling;
	vec2 texCoords = uv * offsetTiling.zw + offsetTiling.xy;
	return textureLod(textures[texIndex], texCoords, 0.0);
}

void main()
{
	// Compute normalized barycentric coordinates from the triangle hit:
    const vec3 bary = vec3(1.0 - hitAttribs.x - hitAttribs.y, hitAttribs.x, hitAttribs.y);

	// Read the custom index that we have stored in struct geometry_instance::mInstanceCustomIndex:
	// We can use it as index into the materials and into the buffer views.
	const int customIndex = nonuniformEXT(gl_InstanceCustomIndexEXT);

	// Read the triangle indices from the index buffer:
	const ivec3 indices = ivec3(texelFetch(indexBuffers[customIndex], gl_PrimitiveID).rgb);

	// Use barycentric coordinates to compute the interpolated uv coordinates:
	const vec2 uv0 = texelFetch(texCoordsBuffers[customIndex], indices.x).st;
	const vec2 uv1 = texelFetch(texCoordsBuffers[customIndex], indices.y).st;
	const vec2 uv2 = texelFetch(texCoordsBuffers[customIndex], indices.z).st;
	const vec2 uv = (bary.x * uv0 + bary.y * uv1 + bary.z * uv2);


	// Use barycentric coordinates to compute the interpolated normals
	const vec3 nrm0 = texelFetch(normalsBuffers[customIndex], indices.x).rgb;
	const vec3 nrm1 = texelFetch(normalsBuffers[customIndex], indices.y).rgb; 
	const vec3 nrm2 = texelFetch(normalsBuffers[customIndex], indices.z).rgb;
	const vec3 normal = (bary.x * nrm0 + bary.y * nrm1 + bary.z * nrm2);

	// Sample color from diffuse texture
	vec3 diffuseTexColor = sample_from_diffuse_texture(customIndex, uv).rgb;

	// Compute diffuse lighting towards light source:
	float nDotL = dot(normal, normalize(pushConstants.mLightDir.xyz));

	// Set diffusely illuminated result as the hitValue:
	hitValue = diffuseTexColor * (max(0.0, nDotL) + pushConstants.mAmbientLight.rgb);

    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 direction = normalize(pushConstants.mLightDir.xyz);
    uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;
    float tmax = 100.0;

	const vec3 hitPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT ;

	if (pushConstants.mEnableShadows) {
		// Produce very simple shadows using recursive ray tracing:
		vec3 rayOrigin = hitPos;
		vec3 rayDirection = pushConstants.mLightDir.xyz;
		float tMin = 0.01;
		float tMax = 1000.0;

		// Initialize with the value we already have, s.t. nothing bad happens at mix() if we didn't hit anything (because the secondary miss shader doesn't modify the value):
		shadowPayload = hitValue; 
		// Our shader binding table (SBT) is structured like follows:
		//  - one ray generation shader
		//  - three closest hit shaders
		//  - two miss shaders
		// We need to get the indices right into these SBT entries by specifying the correct offsets.
		// Not only these offsets take part in the final SBT-index computation, but also the offsets that
		// were specified in the trace_rays(...) call on the CPU-side (but set them to 0 each in this case).
		traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xFF, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, rayOrigin, tMin, rayDirection, tMax, 1 /*payload location*/);

		hitValue = mix(hitValue, shadowPayload, pushConstants.mShadowsFactor);
	}

	if (pushConstants.mEnableAmbientOcclusion) {
		// Produce very simple (and expensive) ambient occlusion using multiple recursive rays:

		vec3 sampleDirections[8] = {
			vec3( 1,  1,  1),
			vec3( 1,  1, -1),
			vec3( 1, -1,  1),
			vec3( 1, -1, -1),
			vec3(-1,  1,  1),
			vec3(-1,  1, -1),
			vec3(-1, -1,  1),
			vec3(-1, -1, -1),
		};

		float ao = 0.0;

		for (int i = 0; i < sampleDirections.length(); ++i) {
			vec3 rayOrigin = hitPos;
			vec3 rayDirection = sampleDirections[i];
			float tMin = pushConstants.mAmbientOcclusionMinDist;
			float tMax = pushConstants.mAmbientOcclusionMaxDist;
			
			// Initialize with the value we already have, s.t. nothing bad happens at mix() if we didn't hit anything (because the secondary miss shader doesn't modify the value):
			aoPayload = 0.0; 
			// Our shader binding table (SBT) is structured like follows:
			//  - one ray generation shader
			//  - three closest hit shaders
			//  - two miss shaders
			// We need to get the indices right into these SBT entries by specifying the correct offsets.
			// Not only these offsets take part in the final SBT-index computation, but also the offsets that
			// were specified in the trace_rays(...) call on the CPU-side (but set them to 0 each in this case).
			traceRayEXT(topLevelAS, gl_RayFlagsNoneEXT, 0xFF, 2 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, rayOrigin, tMin, rayDirection, tMax, 2 /*payload location*/);
			ao += aoPayload;
		}

		ao /= float(sampleDirections.length());

		hitValue = mix(hitValue, pushConstants.mAmbientOcclusionColor.rgb, ao * pushConstants.mAmbientOcclusionFactor);
	}
}
