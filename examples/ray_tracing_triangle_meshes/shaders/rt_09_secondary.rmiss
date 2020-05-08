#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 2) rayPayloadInEXT float secondaryRayHitValue;

void main()
{
    secondaryRayHitValue = gl_RayTmaxEXT;
}