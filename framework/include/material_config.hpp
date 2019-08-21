#pragma once

namespace cgb
{
	/** Contains the raw material config as read out by Assimp.
	 *	Note however, that some fields will not be set by Assimp.
	 */
	struct material_config
	{
		material_config(bool _AlsoConsiderCpuOnlyDataForDistinctMaterials = false)
			: mName{ "some material" }
			, mIgnoreCpuOnlyDataForEquality{ !_AlsoConsiderCpuOnlyDataForDistinctMaterials }
			, mShadingModel{}
			, mWireframeMode{ false }
			, mTwosided{ false }
			, mBlendMode{ cfg::color_blending_config::disable() }
			, mDiffuseReflectivity{ 1.f, 1.f, 1.f, 1.f }
			, mAmbientReflectivity{ 1.f, 1.f, 1.f, 1.f }
			, mSpecularReflectivity{ 1.f, 1.f, 1.f, 1.f }
			, mEmissiveColor{ 0.f, 0.f, 0.f, 0.f }
			, mTransparentColor{ 0.f, 0.f, 0.f, 0.f }
			, mReflectiveColor{ 0.f, 0.f, 0.f, 0.f }
			, mAlbedo{ 0.f, 0.f, 0.f, 0.f }
			, mOpacity{ 1.f }
			, mBumpScaling{ 1.f }
			, mShininess{ 0.f }
			, mShininessStrength{ 0.f }
			, mRefractionIndex{ 0.f }
			, mReflectivity{ 0.f }
			, mMetallic{ 0.f }
			, mSmoothness{ 0.f }
			, mSheen{ 0.f }
			, mThickness{ 0.f }
			, mRoughness{ 0.f }
			, mAnisotropy{ 0.f }
			, mAnisotropyRotation{ 0.f, 0.f, 0.f, 0.f }
			, mCustomData{ 0.f, 0.f, 0.f, 0.f }
			, mDiffuseTex{}
			, mSpecularTex{}
			, mAmbientTex{}
			, mEmissiveTex{}
			, mHeightTex{}
			, mNormalsTex{}
			, mShininessTex{}
			, mOpacityTex{}
			, mDisplacementTex{}
			, mReflectionTex{}
			, mLightmapTex{}
			, mExtraTex{}
			, mDiffuseTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mSpecularTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mAmbientTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mEmissiveTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mHeightTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mNormalsTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mShininessTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mOpacityTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mDisplacementTexOffsetTiling	{ 0.f, 0.f, 1.f, 1.f }
			, mReflectionTexOffsetTiling	{ 0.f, 0.f, 1.f, 1.f }
			, mLightmapTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mExtraTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
		{}

		material_config(material_config&&) = default;
		material_config(const material_config&) = default;
		material_config& operator=(material_config&&) = default;
		material_config& operator=(const material_config&) = default;
		~material_config() = default;

		// CPU-only parameters:
		std::string mName;

		bool mIgnoreCpuOnlyDataForEquality;
		std::string mShadingModel;
		bool mWireframeMode;
		bool mTwosided;
		cfg::color_blending_config mBlendMode;

		// Shader parameters:
		glm::vec4 mDiffuseReflectivity;
		glm::vec4 mAmbientReflectivity;
		glm::vec4 mSpecularReflectivity;
		glm::vec4 mEmissiveColor;
		glm::vec4 mTransparentColor;
		glm::vec4 mReflectiveColor;
		glm::vec4 mAlbedo;

		float mOpacity;
		float mBumpScaling;
		float mShininess;
		float mShininessStrength;

		float mRefractionIndex;
		float mReflectivity;
		float mMetallic;
		float mSmoothness;

		float mSheen;
		float mThickness;
		float mRoughness;
		float mAnisotropy;

		glm::vec4 mAnisotropyRotation;
		glm::vec4 mCustomData;

		std::string mDiffuseTex;
		std::string mSpecularTex;
		std::string mAmbientTex;
		std::string mEmissiveTex;
		std::string mHeightTex;
		std::string mNormalsTex;
		std::string mShininessTex;
		std::string mOpacityTex;
		std::string mDisplacementTex;
		std::string mReflectionTex;
		std::string mLightmapTex;
		std::string mExtraTex;

		glm::vec4 mDiffuseTexOffsetTiling;
		glm::vec4 mSpecularTexOffsetTiling;
		glm::vec4 mAmbientTexOffsetTiling;
		glm::vec4 mEmissiveTexOffsetTiling;
		glm::vec4 mHeightTexOffsetTiling;
		glm::vec4 mNormalsTexOffsetTiling;
		glm::vec4 mShininessTexOffsetTiling;
		glm::vec4 mOpacityTexOffsetTiling;
		glm::vec4 mDisplacementTexOffsetTiling;
		glm::vec4 mReflectionTexOffsetTiling;
		glm::vec4 mLightmapTexOffsetTiling;
		glm::vec4 mExtraTexOffsetTiling;
	};

	/** Compares the two `material_config`s for equality.
	 *	The comparisons made depend on the `mIgnoreCpuOnlyDataForEquality` members. If these members of both
	 *	comparands are `true`, the CPU-only fields will not be compared. Or put in a different way: The two 
	 *	`material`s are considered equal even if they have different values in the GPU-only field.
	 */
	static bool operator ==(const material_config& left, const material_config& right)
	{
		if (!left.mIgnoreCpuOnlyDataForEquality || !right.mIgnoreCpuOnlyDataForEquality) {
			if (left.mShadingModel				!= right.mShadingModel					) return false;
			if (left.mWireframeMode				!= right.mWireframeMode					) return false;
			if (left.mTwosided					!= right.mTwosided						) return false;
			if (left.mBlendMode					!= right.mBlendMode						) return false;
		}
		
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

		if (left.mDiffuseTex					!= right.mDiffuseTex					) return false;
		if (left.mSpecularTex					!= right.mSpecularTex					) return false;
		if (left.mAmbientTex					!= right.mAmbientTex					) return false;
		if (left.mEmissiveTex					!= right.mEmissiveTex					) return false;
		if (left.mHeightTex						!= right.mHeightTex						) return false;
		if (left.mNormalsTex					!= right.mNormalsTex					) return false;
		if (left.mShininessTex					!= right.mShininessTex					) return false;
		if (left.mOpacityTex					!= right.mOpacityTex					) return false;
		if (left.mDisplacementTex				!= right.mDisplacementTex				) return false;
		if (left.mReflectionTex					!= right.mReflectionTex					) return false;
		if (left.mLightmapTex					!= right.mLightmapTex					) return false;
		if (left.mExtraTex						!= right.mExtraTex						) return false;

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
	static bool operator !=(const material_config& left, const material_config& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `cgb::material_config` into std::
{
	template<> struct hash<cgb::material_config>
	{
		std::size_t operator()(cgb::material_config const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h,
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
				o.mDiffuseTex,
				o.mSpecularTex,
				o.mAmbientTex,
				o.mEmissiveTex,
				o.mHeightTex,
				o.mNormalsTex,
				o.mShininessTex,
				o.mOpacityTex,
				o.mDisplacementTex,
				o.mReflectionTex,
				o.mLightmapTex,
				o.mExtraTex,
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
			if (!o.mIgnoreCpuOnlyDataForEquality) {
				cgb::hash_combine(h,
					o.mShadingModel,
					o.mWireframeMode,
					o.mTwosided,
					o.mBlendMode
				);
			}
			return h;
		}
	};
}
