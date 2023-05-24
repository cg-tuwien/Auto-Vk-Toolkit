#pragma once

namespace avk
{
	/** Material data in the right format to be uploaded to the GPU
	 *	and to be used in a GPU buffer like a UBO or an SSBO.
	 *
	 *	Possible corresponding GLSL data structures:
	 *	
	 *	layout(set = 0, binding = 0) uniform sampler2D textures[];
	 *
	 *	struct MaterialGpuData
	 *	{
	 *		vec4 mDiffuseReflectivity;
	 *		vec4 mAmbientReflectivity;
	 *		vec4 mSpecularReflectivity;
	 *		vec4 mEmissiveColor;
	 *		vec4 mTransparentColor;
	 *		vec4 mReflectiveColor;
	 *		vec4 mAlbedo;
	 *
	 *		float mOpacity;
	 *		float mBumpScaling;
	 *		float mShininess;
	 *		float mShininessStrength;
	 *		
	 *		float mRefractionIndex;
	 *		float mReflectivity;
	 *		float mMetallic;
	 *		float mSmoothness;
	 *		
	 *		float mSheen;
	 *		float mThickness;
	 *		float mRoughness;
	 *		float mAnisotropy;
	 *		
	 *		vec4 mAnisotropyRotation;
	 *		vec4 mCustomData;
	 *		
	 *		int mDiffuseTexIndex;
	 *		int mSpecularTexIndex;
	 *		int mAmbientTexIndex;
	 *		int mEmissiveTexIndex;
	 *		int mHeightTexIndex;
	 *		int mNormalsTexIndex;
	 *		int mShininessTexIndex;
	 *		int mOpacityTexIndex;
	 *		int mDisplacementTexIndex;
	 *		int mReflectionTexIndex;
	 *		int mLightmapTexIndex;
	 *		int mExtraTexIndex;
	 *		
	 *		vec4 mDiffuseTexOffsetTiling;
	 *		vec4 mSpecularTexOffsetTiling;
	 *		vec4 mAmbientTexOffsetTiling;
	 *		vec4 mEmissiveTexOffsetTiling;
	 *		vec4 mHeightTexOffsetTiling;
	 *		vec4 mNormalsTexOffsetTiling;
	 *		vec4 mShininessTexOffsetTiling;
	 *		vec4 mOpacityTexOffsetTiling;
	 *		vec4 mDisplacementTexOffsetTiling;
	 *		vec4 mReflectionTexOffsetTiling;
	 *		vec4 mLightmapTexOffsetTiling;
	 *		vec4 mExtraTexOffsetTiling;
	 *	};
	 *
	 *	layout(set = 0, binding = 1) buffer Material 
	 *	{
	 *		MaterialGpuData materials[];
	 *	} materialsBuffer;
	 *	
	 */
	struct material_gpu_data
	{
		alignas(16) glm::vec4 mDiffuseReflectivity;
		alignas(16) glm::vec4 mAmbientReflectivity;
		alignas(16) glm::vec4 mSpecularReflectivity;
		alignas(16) glm::vec4 mEmissiveColor;
		alignas(16) glm::vec4 mTransparentColor;
		alignas(16) glm::vec4 mReflectiveColor;
		alignas(16) glm::vec4 mAlbedo;

		alignas(4) float mOpacity;
		alignas(4) float mBumpScaling;
		alignas(4) float mShininess;
		alignas(4) float mShininessStrength;

		alignas(4) float mRefractionIndex;
		alignas(4) float mReflectivity;
		alignas(4) float mMetallic;
		alignas(4) float mSmoothness;

		alignas(4) float mSheen;
		alignas(4) float mThickness;
		alignas(4) float mRoughness;
		alignas(4) float mAnisotropy;

		alignas(16) glm::vec4 mAnisotropyRotation;
		alignas(16) glm::vec4 mCustomData;

		alignas(4) int32_t mDiffuseTexIndex;
		alignas(4) int32_t mSpecularTexIndex;
		alignas(4) int32_t mAmbientTexIndex;
		alignas(4) int32_t mEmissiveTexIndex;
		alignas(4) int32_t mHeightTexIndex;
		alignas(4) int32_t mNormalsTexIndex;
		alignas(4) int32_t mShininessTexIndex;
		alignas(4) int32_t mOpacityTexIndex;
		alignas(4) int32_t mDisplacementTexIndex;
		alignas(4) int32_t mReflectionTexIndex;
		alignas(4) int32_t mLightmapTexIndex;
		alignas(4) int32_t mExtraTexIndex;

		alignas(16) glm::vec4 mDiffuseTexOffsetTiling;
		alignas(16) glm::vec4 mSpecularTexOffsetTiling;
		alignas(16) glm::vec4 mAmbientTexOffsetTiling;
		alignas(16) glm::vec4 mEmissiveTexOffsetTiling;
		alignas(16) glm::vec4 mHeightTexOffsetTiling;
		alignas(16) glm::vec4 mNormalsTexOffsetTiling;
		alignas(16) glm::vec4 mShininessTexOffsetTiling;
		alignas(16) glm::vec4 mOpacityTexOffsetTiling;
		alignas(16) glm::vec4 mDisplacementTexOffsetTiling;
		alignas(16) glm::vec4 mReflectionTexOffsetTiling;
		alignas(16) glm::vec4 mLightmapTexOffsetTiling;
		alignas(16) glm::vec4 mExtraTexOffsetTiling;
	};

	/** Compares the two `material_gpu_data`s for equality.
	 */
	static bool operator ==(const material_gpu_data& left, const material_gpu_data& right)
	{
		if (left.mDiffuseReflectivity			!= right.mDiffuseReflectivity			) return false;
		if (left.mAmbientReflectivity			!= right.mAmbientReflectivity			) return false;
		if (left.mSpecularReflectivity			!= right.mSpecularReflectivity			) return false;
		if (left.mEmissiveColor					!= right.mEmissiveColor					) return false;
		if (left.mTransparentColor				!= right.mTransparentColor				) return false;
		if (left.mReflectiveColor				!= right.mReflectiveColor				) return false;
		if (left.mAlbedo						!= right.mAlbedo						) return false;

		if (left.mRefractionIndex				!= right.mRefractionIndex				) return false;
		if (left.mReflectivity					!= right.mReflectivity					) return false;
		if (left.mMetallic						!= right.mMetallic						) return false;
		if (left.mSmoothness					!= right.mSmoothness					) return false;

		if (left.mRefractionIndex				!= right.mRefractionIndex				) return false;
		if (left.mReflectivity					!= right.mReflectivity					) return false;
		if (left.mMetallic						!= right.mMetallic						) return false;
		if (left.mSmoothness					!= right.mSmoothness					) return false;

		if (left.mSheen							!= right.mSheen							) return false;
		if (left.mThickness						!= right.mThickness						) return false;
		if (left.mRoughness						!= right.mRoughness						) return false;
		if (left.mAnisotropy					!= right.mAnisotropy					) return false;

		if (left.mAnisotropyRotation			!= right.mAnisotropyRotation			) return false;
		if (left.mCustomData					!= right.mCustomData					) return false;

		if (left.mDiffuseTexIndex				!= right.mDiffuseTexIndex				) return false;
		if (left.mSpecularTexIndex				!= right.mSpecularTexIndex				) return false;
		if (left.mAmbientTexIndex				!= right.mAmbientTexIndex				) return false;
		if (left.mEmissiveTexIndex				!= right.mEmissiveTexIndex				) return false;
		if (left.mHeightTexIndex				!= right.mHeightTexIndex				) return false;
		if (left.mNormalsTexIndex				!= right.mNormalsTexIndex				) return false;
		if (left.mShininessTexIndex				!= right.mShininessTexIndex				) return false;
		if (left.mOpacityTexIndex				!= right.mOpacityTexIndex				) return false;
		if (left.mDisplacementTexIndex			!= right.mDisplacementTexIndex			) return false;
		if (left.mReflectionTexIndex			!= right.mReflectionTexIndex			) return false;
		if (left.mLightmapTexIndex				!= right.mLightmapTexIndex				) return false;
		if (left.mExtraTexIndex					!= right.mExtraTexIndex					) return false;

		if (left.mDiffuseTexOffsetTiling		!= right.mDiffuseTexOffsetTiling		) return false;
		if (left.mSpecularTexOffsetTiling		!= right.mSpecularTexOffsetTiling		) return false;
		if (left.mAmbientTexOffsetTiling		!= right.mAmbientTexOffsetTiling		) return false;
		if (left.mEmissiveTexOffsetTiling		!= right.mEmissiveTexOffsetTiling		) return false;
		if (left.mHeightTexOffsetTiling			!= right.mHeightTexOffsetTiling			) return false;
		if (left.mNormalsTexOffsetTiling		!= right.mNormalsTexOffsetTiling		) return false;
		if (left.mShininessTexOffsetTiling		!= right.mShininessTexOffsetTiling		) return false;
		if (left.mOpacityTexOffsetTiling		!= right.mOpacityTexOffsetTiling		) return false;
		if (left.mDisplacementTexOffsetTiling	!= right.mDisplacementTexOffsetTiling	) return false;
		if (left.mReflectionTexOffsetTiling		!= right.mReflectionTexOffsetTiling		) return false;
		if (left.mLightmapTexOffsetTiling		!= right.mLightmapTexOffsetTiling		) return false;
		if (left.mExtraTexOffsetTiling			!= right.mExtraTexOffsetTiling			) return false;

		return true;
	}

	/** Negated result of operator==, see the equality operator for more details! */
	static bool operator !=(const material_gpu_data& left, const material_gpu_data& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `avk::material_gpu_data` into std::
{
	template<> struct hash<avk::material_gpu_data>
	{
		std::size_t operator()(avk::material_gpu_data const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				o.mDiffuseReflectivity,
				o.mAmbientReflectivity,
				o.mSpecularReflectivity,
				o.mEmissiveColor,
				o.mTransparentColor,
				o.mReflectiveColor,
				o.mAlbedo,
				o.mOpacity,
				o.mBumpScaling,
				o.mShininess,
				o.mShininessStrength,
				o.mRefractionIndex,
				o.mReflectivity,
				o.mMetallic,
				o.mSmoothness,
				o.mSheen,
				o.mThickness,
				o.mRoughness,
				o.mAnisotropy,
				o.mAnisotropyRotation,
				o.mCustomData,
				o.mDiffuseTexIndex,
				o.mSpecularTexIndex,
				o.mAmbientTexIndex,
				o.mEmissiveTexIndex,
				o.mHeightTexIndex,
				o.mNormalsTexIndex,
				o.mShininessTexIndex,
				o.mOpacityTexIndex,
				o.mDisplacementTexIndex,
				o.mReflectionTexIndex,
				o.mLightmapTexIndex,
				o.mExtraTexIndex,
				o.mDiffuseTexOffsetTiling,
				o.mSpecularTexOffsetTiling,
				o.mAmbientTexOffsetTiling,
				o.mEmissiveTexOffsetTiling,
				o.mHeightTexOffsetTiling,
				o.mNormalsTexOffsetTiling,
				o.mShininessTexOffsetTiling,
				o.mOpacityTexOffsetTiling,
				o.mDisplacementTexOffsetTiling,
				o.mReflectionTexOffsetTiling,
				o.mLightmapTexOffsetTiling,
				o.mExtraTexOffsetTiling
			);
			return h;
		}
	};
}
