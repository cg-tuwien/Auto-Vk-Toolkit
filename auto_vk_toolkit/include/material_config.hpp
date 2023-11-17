#pragma once

namespace avk
{
	/** Contains the raw material config as read out by Assimp.
	 *	Note however, that some fields will not be set by Assimp.
	 */
	struct material_config
	{
		material_config(bool _AlsoConsiderCpuOnlyDataForDistinctMaterials = false) noexcept
			: mName{ "some material" }
			, mIgnoreCpuOnlyDataForEquality{ !_AlsoConsiderCpuOnlyDataForDistinctMaterials }
			, mShadingModel{}
			, mWireframeMode{ false }
			, mTwosided{ false }
			, mBlendMode{ avk::cfg::color_blending_config::disable() }
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
			, mDiffuseTexUvSet					{ 0 }
			, mSpecularTexUvSet					{ 0 }
			, mAmbientTexUvSet					{ 0 }
			, mEmissiveTexUvSet					{ 0 }
			, mHeightTexUvSet					{ 0 }
			, mNormalsTexUvSet					{ 0 }
			, mShininessTexUvSet				{ 0 }
			, mOpacityTexUvSet					{ 0 }
			, mDisplacementTexUvSet				{ 0 }
			, mReflectionTexUvSet				{ 0 }
			, mLightmapTexUvSet					{ 0 }
			, mExtraTexUvSet					{ 0 }
			, mDiffuseTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mSpecularTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mAmbientTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mEmissiveTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mHeightTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mNormalsTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mShininessTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mOpacityTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mDisplacementTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mReflectionTexOffsetTiling		{ 0.f, 0.f, 1.f, 1.f }
			, mLightmapTexOffsetTiling			{ 0.f, 0.f, 1.f, 1.f }
			, mExtraTexOffsetTiling				{ 0.f, 0.f, 1.f, 1.f }
			, mDiffuseTexRotation				{ 0.0f }
			, mSpecularTexRotation				{ 0.0f }
			, mAmbientTexRotation				{ 0.0f }
			, mEmissiveTexRotation				{ 0.0f }
			, mHeightTexRotation				{ 0.0f }
			, mNormalsTexRotation				{ 0.0f }
			, mShininessTexRotation				{ 0.0f }
			, mOpacityTexRotation				{ 0.0f }
			, mDisplacementTexRotation			{ 0.0f }
			, mReflectionTexRotation			{ 0.0f }
			, mLightmapTexRotation				{ 0.0f }
			, mExtraTexRotation					{ 0.0f }
			, mDiffuseTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mSpecularTexBorderHandlingMode	{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mAmbientTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mEmissiveTexBorderHandlingMode	{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mHeightTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mNormalsTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mShininessTexBorderHandlingMode	{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mOpacityTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mDisplacementTexBorderHandlingMode{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mReflectionTexBorderHandlingMode	{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mLightmapTexBorderHandlingMode	{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
			, mExtraTexBorderHandlingMode		{ { avk::border_handling_mode::clamp_to_edge, avk::border_handling_mode::clamp_to_edge } }
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
		avk::cfg::color_blending_config mBlendMode;

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

		uint32_t    mDiffuseTexUvSet;
		uint32_t    mSpecularTexUvSet;
		uint32_t    mAmbientTexUvSet;
		uint32_t    mEmissiveTexUvSet;
		uint32_t    mHeightTexUvSet;
		uint32_t    mNormalsTexUvSet;
		uint32_t    mShininessTexUvSet;
		uint32_t    mOpacityTexUvSet;
		uint32_t    mDisplacementTexUvSet;
		uint32_t    mReflectionTexUvSet;
		uint32_t    mLightmapTexUvSet;
		uint32_t    mExtraTexUvSet;

		glm::vec4   mDiffuseTexOffsetTiling;
		glm::vec4   mSpecularTexOffsetTiling;
		glm::vec4   mAmbientTexOffsetTiling;
		glm::vec4   mEmissiveTexOffsetTiling;
		glm::vec4   mHeightTexOffsetTiling;
		glm::vec4   mNormalsTexOffsetTiling;
		glm::vec4   mShininessTexOffsetTiling;
		glm::vec4   mOpacityTexOffsetTiling;
		glm::vec4   mDisplacementTexOffsetTiling;
		glm::vec4   mReflectionTexOffsetTiling;
		glm::vec4   mLightmapTexOffsetTiling;
		glm::vec4   mExtraTexOffsetTiling;
		
		float       mDiffuseTexRotation;
		float       mSpecularTexRotation;
		float       mAmbientTexRotation;
		float       mEmissiveTexRotation;
		float       mHeightTexRotation;
		float       mNormalsTexRotation;
		float       mShininessTexRotation;
		float       mOpacityTexRotation;
		float       mDisplacementTexRotation;
		float       mReflectionTexRotation;
		float       mLightmapTexRotation;
		float       mExtraTexRotation;

		// CPU-only parameters again:
		// Border handling modes: U, V, in that order.
		std::array<avk::border_handling_mode, 2> mDiffuseTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mSpecularTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mAmbientTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mEmissiveTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mHeightTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mNormalsTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mShininessTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mOpacityTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mDisplacementTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mReflectionTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mLightmapTexBorderHandlingMode;
		std::array<avk::border_handling_mode, 2> mExtraTexBorderHandlingMode;
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

		if (left.mDiffuseTexUvSet				!= right.mDiffuseTexUvSet				) return false;
		if (left.mSpecularTexUvSet				!= right.mSpecularTexUvSet				) return false;
		if (left.mAmbientTexUvSet				!= right.mAmbientTexUvSet				) return false;
		if (left.mEmissiveTexUvSet				!= right.mEmissiveTexUvSet				) return false;
		if (left.mHeightTexUvSet				!= right.mHeightTexUvSet				) return false;
		if (left.mNormalsTexUvSet				!= right.mNormalsTexUvSet				) return false;
		if (left.mShininessTexUvSet				!= right.mShininessTexUvSet				) return false;
		if (left.mOpacityTexUvSet				!= right.mOpacityTexUvSet				) return false;
		if (left.mDisplacementTexUvSet			!= right.mDisplacementTexUvSet			) return false;
		if (left.mReflectionTexUvSet			!= right.mReflectionTexUvSet			) return false;
		if (left.mLightmapTexUvSet				!= right.mLightmapTexUvSet				) return false;
		if (left.mExtraTexUvSet					!= right.mExtraTexUvSet					) return false;

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

		if (left.mDiffuseTexRotation			!= right.mDiffuseTexRotation			) return false;
		if (left.mSpecularTexRotation			!= right.mSpecularTexRotation			) return false;
		if (left.mAmbientTexRotation			!= right.mAmbientTexRotation			) return false;
		if (left.mEmissiveTexRotation			!= right.mEmissiveTexRotation			) return false;
		if (left.mHeightTexRotation				!= right.mHeightTexRotation				) return false;
		if (left.mNormalsTexRotation			!= right.mNormalsTexRotation			) return false;
		if (left.mShininessTexRotation			!= right.mShininessTexRotation			) return false;
		if (left.mOpacityTexRotation			!= right.mOpacityTexRotation			) return false;
		if (left.mDisplacementTexRotation		!= right.mDisplacementTexRotation		) return false;
		if (left.mReflectionTexRotation			!= right.mReflectionTexRotation			) return false;
		if (left.mLightmapTexRotation			!= right.mLightmapTexRotation			) return false;
		if (left.mExtraTexRotation				!= right.mExtraTexRotation				) return false;

		if (!left.mIgnoreCpuOnlyDataForEquality || !right.mIgnoreCpuOnlyDataForEquality) {
			for (int i=0; i < 2; ++i) {
				if (left.mDiffuseTexBorderHandlingMode[i]		!= right.mDiffuseTexBorderHandlingMode[i]		) return false;
				if (left.mSpecularTexBorderHandlingMode[i]		!= right.mSpecularTexBorderHandlingMode[i]		) return false;
				if (left.mAmbientTexBorderHandlingMode[i]		!= right.mAmbientTexBorderHandlingMode[i]		) return false;
				if (left.mEmissiveTexBorderHandlingMode[i]		!= right.mEmissiveTexBorderHandlingMode[i]		) return false;
				if (left.mHeightTexBorderHandlingMode[i]		!= right.mHeightTexBorderHandlingMode[i]		) return false;
				if (left.mNormalsTexBorderHandlingMode[i]		!= right.mNormalsTexBorderHandlingMode[i]		) return false;
				if (left.mShininessTexBorderHandlingMode[i]		!= right.mShininessTexBorderHandlingMode[i]		) return false;
				if (left.mOpacityTexBorderHandlingMode[i]		!= right.mOpacityTexBorderHandlingMode[i]		) return false;
				if (left.mDisplacementTexBorderHandlingMode[i]	!= right.mDisplacementTexBorderHandlingMode[i]	) return false;
				if (left.mReflectionTexBorderHandlingMode[i]	!= right.mReflectionTexBorderHandlingMode[i]	) return false;
				if (left.mLightmapTexBorderHandlingMode[i]		!= right.mLightmapTexBorderHandlingMode[i]		) return false;
				if (left.mExtraTexBorderHandlingMode[i]			!= right.mExtraTexBorderHandlingMode[i]			) return false;
			}
		}
		
		return true;
	}

	/** Negated result of operator==, see the equality operator for more details! */
	static bool operator !=(const material_config& left, const material_config& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `avk::material_config` into std::
{
	template<> struct hash<avk::material_config>
	{
		std::size_t operator()(avk::material_config const& o) const noexcept
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
				o.mDiffuseTexUvSet,
				o.mSpecularTexUvSet,
				o.mAmbientTexUvSet,
				o.mEmissiveTexUvSet,
				o.mHeightTexUvSet,
				o.mNormalsTexUvSet,
				o.mShininessTexUvSet,
				o.mOpacityTexUvSet,
				o.mDisplacementTexUvSet,
				o.mReflectionTexUvSet,
				o.mLightmapTexUvSet,
				o.mExtraTexUvSet,
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
				o.mExtraTexOffsetTiling,
				o.mDiffuseTexRotation,
				o.mSpecularTexRotation,
				o.mAmbientTexRotation,
				o.mEmissiveTexRotation,
				o.mHeightTexRotation,
				o.mNormalsTexRotation,
				o.mShininessTexRotation,
				o.mOpacityTexRotation,
				o.mDisplacementTexRotation,
				o.mReflectionTexRotation,
				o.mLightmapTexRotation,
				o.mExtraTexRotation
			);
			if (!o.mIgnoreCpuOnlyDataForEquality) {
				avk::hash_combine(h,
					o.mShadingModel,
					o.mWireframeMode,
					o.mTwosided,
					o.mBlendMode
				);
				for (int i = 0; i < 2; ++i) {
					avk::hash_combine(h,
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mDiffuseTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mSpecularTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mAmbientTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mEmissiveTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mHeightTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mNormalsTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mShininessTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mOpacityTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mDisplacementTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mReflectionTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mLightmapTexBorderHandlingMode[i]),
						static_cast<std::underlying_type<avk::border_handling_mode>::type>(o.mExtraTexBorderHandlingMode[i])
					);
				}
			}
			return h;
		}
	};
}
