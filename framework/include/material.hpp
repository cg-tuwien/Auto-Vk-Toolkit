#pragma once

namespace cgb
{
	struct material
	{
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

namespace std // Inject hash for `cgb::material` into std::
{
	template<> struct hash<cgb::material>
	{
		std::size_t operator()(cgb::material const& o) const noexcept
		{
			auto& config = o.mMaterialConfig;

			std::size_t h = 0;
			cgb::hash_combine(h,
				config.mDiffuseReflectivity,
				config.mAmbientReflectivity,
				config.mSpecularReflectivity,
				config.mEmissiveColor,
				config.mTransparentColor,
				config.mReflectiveColor,
				config.mAlbedo,
				config.mOpacity,
				config.mBumpScaling,
				config.mShininess,
				config.mShininessStrength,
				config.mRefractionIndex,
				config.mReflectivity,
				config.mMetallic,
				config.mSmoothness,
				config.mSheen,
				config.mThickness,
				config.mRoughness,
				config.mAnisotropy,
				config.mAnisotropyRotation,
				config.mCustomData,
				*o.mDiffuseTexImageSampler,
				*o.mSpecularTexImageSampler,
				*o.mAmbientTexImageSampler,
				*o.mEmissiveTexImageSampler,
				*o.mHeightTexImageSampler,
				*o.mNormalsTexImageSampler,
				*o.mShininessTexImageSampler,
				*o.mOpacityTexImageSampler,
				*o.mDisplacementTexImageSampler,
				*o.mReflectionTexImageSampler,
				*o.mLightmapTexImageSampler,
				*o.mExtraTexImageSampler,
				config.mDiffuseTexOffsetTiling,
				config.mSpecularTexOffsetTiling,
				config.mAmbientTexOffsetTiling,
				config.mEmissiveTexOffsetTiling,
				config.mHeightTexOffsetTiling,
				config.mNormalsTexOffsetTiling,
				config.mShininessTexOffsetTiling,
				config.mOpacityTexOffsetTiling,
				config.mDisplacementTexOffsetTiling,
				config.mReflectionTexOffsetTiling,
				config.mLightmapTexOffsetTiling,
				config.mExtraTexOffsetTiling
			);
			if (!config.mIgnoreCpuOnlyDataForHash) {
				cgb::hash_combine(h,
					config.mShadingModel,
					config.mWireframeMode,
					config.mTwosided,
					config.mBlendMode
				);
			}
			return h;
		}
	};
}
