#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 2) rayPayloadInEXT float aoPayload;

void main()
{
    aoPayload = 1.0;
}
