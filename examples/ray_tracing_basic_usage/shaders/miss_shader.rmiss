#version 460
#extension GL_EXT_ray_tracing : require

layout(location = 0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = vec3(0.0, 0.1, 0.3);
}
//#version 460 core
//#extension GL_EXT_ray_tracing : enable
//
//layout(location = 0) rayPayloadInEXT vec4 payload;
//
//void main() {
//  payload = vec4(0.3);
//}
//