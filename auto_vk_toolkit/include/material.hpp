#pragma once

namespace avk
{
	/** Material struct which hold all the concrete data, i.e. 
	 *	in addition to the data in material config also all the 
	 *	`image_sampler` instances.
	 */
	struct material
	{
		material_config mMaterialConfig;
		avk::image_sampler mDiffuseTexImageSampler;
		avk::image_sampler mSpecularTexImageSampler;
		avk::image_sampler mAmbientTexImageSampler;
		avk::image_sampler mEmissiveTexImageSampler;
		avk::image_sampler mHeightTexImageSampler;
		avk::image_sampler mNormalsTexImageSampler;
		avk::image_sampler mShininessTexImageSampler;
		avk::image_sampler mOpacityTexImageSampler;
		avk::image_sampler mDisplacementTexImageSampler;
		avk::image_sampler mReflectionTexImageSampler;
		avk::image_sampler mLightmapTexImageSampler;
		avk::image_sampler mExtraTexImageSampler;
	};

	/** Compares the two `material`s for equality.
	 *	The comparisons made depend on the `mIgnoreCpuOnlyDataForEquality` members. If these members of both
	 *	comparands are `true`, the CPU-only fields will not be compared. Or put in a different way: The two 
	 *	`material`s are considered equal even if they have different values in the GPU-only field.
	 */
	static bool operator ==(const material& left, const material& right)
	{
		if (!left.mMaterialConfig.mIgnoreCpuOnlyDataForEquality || !right.mMaterialConfig.mIgnoreCpuOnlyDataForEquality) {
			if (left.mMaterialConfig.mShadingModel				!= right.mMaterialConfig.mShadingModel					) return false;
			if (left.mMaterialConfig.mWireframeMode				!= right.mMaterialConfig.mWireframeMode					) return false;
			if (left.mMaterialConfig.mTwosided					!= right.mMaterialConfig.mTwosided						) return false;
			if (left.mMaterialConfig.mBlendMode					!= right.mMaterialConfig.mBlendMode						) return false;
		}
		
		if (left.mMaterialConfig.mDiffuseReflectivity			!= right.mMaterialConfig.mDiffuseReflectivity			) return false;
		if (left.mMaterialConfig.mAmbientReflectivity			!= right.mMaterialConfig.mAmbientReflectivity			) return false;
		if (left.mMaterialConfig.mSpecularReflectivity			!= right.mMaterialConfig.mSpecularReflectivity			) return false;
		if (left.mMaterialConfig.mEmissiveColor					!= right.mMaterialConfig.mEmissiveColor					) return false;
		if (left.mMaterialConfig.mTransparentColor				!= right.mMaterialConfig.mTransparentColor				) return false;
		if (left.mMaterialConfig.mReflectiveColor				!= right.mMaterialConfig.mReflectiveColor				) return false;
		if (left.mMaterialConfig.mAlbedo						!= right.mMaterialConfig.mAlbedo						) return false;

		if (left.mMaterialConfig.mRefractionIndex				!= right.mMaterialConfig.mRefractionIndex				) return false;
		if (left.mMaterialConfig.mReflectivity					!= right.mMaterialConfig.mReflectivity					) return false;
		if (left.mMaterialConfig.mMetallic						!= right.mMaterialConfig.mMetallic						) return false;
		if (left.mMaterialConfig.mSmoothness					!= right.mMaterialConfig.mSmoothness					) return false;

		if (left.mMaterialConfig.mRefractionIndex				!= right.mMaterialConfig.mRefractionIndex				) return false;
		if (left.mMaterialConfig.mReflectivity					!= right.mMaterialConfig.mReflectivity					) return false;
		if (left.mMaterialConfig.mMetallic						!= right.mMaterialConfig.mMetallic						) return false;
		if (left.mMaterialConfig.mSmoothness					!= right.mMaterialConfig.mSmoothness					) return false;

		if (left.mMaterialConfig.mSheen							!= right.mMaterialConfig.mSheen							) return false;
		if (left.mMaterialConfig.mThickness						!= right.mMaterialConfig.mThickness						) return false;
		if (left.mMaterialConfig.mRoughness						!= right.mMaterialConfig.mRoughness						) return false;
		if (left.mMaterialConfig.mAnisotropy					!= right.mMaterialConfig.mAnisotropy					) return false;

		if (left.mMaterialConfig.mAnisotropyRotation			!= right.mMaterialConfig.mAnisotropyRotation			) return false;
		if (left.mMaterialConfig.mCustomData					!= right.mMaterialConfig.mCustomData					) return false;

		if (*left.mDiffuseTexImageSampler						!= *right.mDiffuseTexImageSampler						) return false;
		if (*left.mSpecularTexImageSampler						!= *right.mSpecularTexImageSampler						) return false;
		if (*left.mAmbientTexImageSampler						!= *right.mAmbientTexImageSampler						) return false;
		if (*left.mEmissiveTexImageSampler						!= *right.mEmissiveTexImageSampler						) return false;
		if (*left.mHeightTexImageSampler						!= *right.mHeightTexImageSampler						) return false;
		if (*left.mNormalsTexImageSampler						!= *right.mNormalsTexImageSampler						) return false;
		if (*left.mShininessTexImageSampler						!= *right.mShininessTexImageSampler						) return false;
		if (*left.mOpacityTexImageSampler						!= *right.mOpacityTexImageSampler						) return false;
		if (*left.mDisplacementTexImageSampler					!= *right.mDisplacementTexImageSampler					) return false;
		if (*left.mReflectionTexImageSampler					!= *right.mReflectionTexImageSampler					) return false;
		if (*left.mLightmapTexImageSampler						!= *right.mLightmapTexImageSampler						) return false;
		if (*left.mExtraTexImageSampler							!= *right.mExtraTexImageSampler							) return false;

		if (left.mMaterialConfig.mDiffuseTexOffsetTiling		!= right.mMaterialConfig.mDiffuseTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mSpecularTexOffsetTiling		!= right.mMaterialConfig.mSpecularTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mAmbientTexOffsetTiling		!= right.mMaterialConfig.mAmbientTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mEmissiveTexOffsetTiling		!= right.mMaterialConfig.mEmissiveTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mHeightTexOffsetTiling			!= right.mMaterialConfig.mHeightTexOffsetTiling			) return false;
		if (left.mMaterialConfig.mNormalsTexOffsetTiling		!= right.mMaterialConfig.mNormalsTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mShininessTexOffsetTiling		!= right.mMaterialConfig.mShininessTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mOpacityTexOffsetTiling		!= right.mMaterialConfig.mOpacityTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mDisplacementTexOffsetTiling	!= right.mMaterialConfig.mDisplacementTexOffsetTiling	) return false;
		if (left.mMaterialConfig.mReflectionTexOffsetTiling		!= right.mMaterialConfig.mReflectionTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mLightmapTexOffsetTiling		!= right.mMaterialConfig.mLightmapTexOffsetTiling		) return false;
		if (left.mMaterialConfig.mExtraTexOffsetTiling			!= right.mMaterialConfig.mExtraTexOffsetTiling			) return false;

		return true;
	}

	/** Negated result of operator==, see the equality operator for more details! */
	static bool operator !=(const material& left, const material& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `avk::material` into std::
{
	template<> struct hash<avk::material>
	{
		std::size_t operator()(avk::material const& o) const noexcept
		{
			auto& config = o.mMaterialConfig;

			std::size_t h = 0;
			avk::hash_combine(h,
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
			if (!config.mIgnoreCpuOnlyDataForEquality) {
				avk::hash_combine(h,
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
