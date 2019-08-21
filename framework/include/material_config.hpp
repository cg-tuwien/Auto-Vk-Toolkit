#pragma once

namespace cgb
{
	struct material_config
	{
		material_config()
			: mName{ "some material" }
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
}
