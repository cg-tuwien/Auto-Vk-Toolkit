#version 460
#extension GL_NV_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform accelerationStructureNV topLevelAS;

layout(location = 0) rayPayloadInNV vec3 hitValue;
hitAttributeNV vec3 attribs;
layout(location = 2) rayPayloadNV float secondaryRayHitValue;

layout(push_constant) uniform PushConstants {
	mat4 viewMatrix;
    vec4 lightDir;
} pushConstants;

void main()
{
    vec3 origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV;
    vec3 direction = normalize(pushConstants.lightDir.xyz);
    uint rayFlags = gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV;
    uint cullMask = 0xff;
    float tmin = 0.001;
    float tmax = 100.0;

//    traceNV(topLevelAS, rayFlags, cullMask, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, origin, tmin, direction, tmax, 2 /*payload location*/);

	hitValue = vec3(0.5, 0.5, 0.5); // * (secondaryRayHitValue < tmax ? 0.25 : 1.0);
}