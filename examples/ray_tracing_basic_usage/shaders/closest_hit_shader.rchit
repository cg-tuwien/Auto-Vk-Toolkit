#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 1, binding = 0) uniform accelerationStructureEXT topLevelAS;

layout(location = 0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec3 attribs;
layout(location = 2) rayPayloadEXT float secondaryRayHitValue;

layout(push_constant) uniform PushConstants {
	mat4 viewMatrix;
    vec4 lightDir;
} pushConstants;

void main()
{
    vec3 origin = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 direction = normalize(pushConstants.lightDir.xyz);
    uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT;
    uint cullMask = 0xff;
    float tmin = 0.001;
    float tmax = 100.0;

//    traceNV(topLevelAS, rayFlags, cullMask, 1 /* sbtRecordOffset */, 0 /* sbtRecordStride */, 1 /* missIndex */, origin, tmin, direction, tmax, 2 /*payload location*/);

	hitValue = vec3(0.5, 0.5, 0.5); // * (secondaryRayHitValue < tmax ? 0.25 : 1.0);
}