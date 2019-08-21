#pragma once

namespace cgb
{
	class material
	{
		
	private:
		material_config mMaterialConfig;
		image_sampler mDiffuseTexImageSampler;
		image_sampler mSpecularTexImageSampler;
		image_sampler mAmbientTexImageSampler;
		image_sampler mEmissiveTexImageSampler;
		image_sampler mHeightTexImageSampler;
		image_sampler mNormalsTexImageSampler;
		image_sampler mShininessTexImageSampler;
		image_sampler mOpacityTexImageSampler;
		image_sampler mDisplacementTexImageSampler;
		image_sampler mReflectionTexImageSampler;
		image_sampler mLightmapTexImageSampler;
		image_sampler mExtraTexImageSampler;
	};
}
