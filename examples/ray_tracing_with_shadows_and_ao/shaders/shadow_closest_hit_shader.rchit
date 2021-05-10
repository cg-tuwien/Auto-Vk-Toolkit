#version 460
#extension GL_EXT_ray_tracing : require

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

layout(location = 1) rayPayloadInEXT vec3 shadowPayload;

void main()
{
    shadowPayload = pushConstants.mShadowsColor.rgb;
}
