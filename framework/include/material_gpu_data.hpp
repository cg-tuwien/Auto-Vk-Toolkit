#pragma once

namespace cgb
{
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

		alignas(4) int mDiffuseTexIndex;
		alignas(4) int mSpecularTexIndex;
		alignas(4) int mAmbientTexIndex;
		alignas(4) int mEmissiveTexIndex;
		alignas(4) int mHeightTexIndex;
		alignas(4) int mNormalsTexIndex;
		alignas(4) int mShininessTexIndex;
		alignas(4) int mOpacityTexIndex;
		alignas(4) int mDisplacementTexIndex;
		alignas(4) int mReflectionTexIndex;
		alignas(4) int mLightmapTexIndex;
		alignas(4) int mExtraTexIndex;

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
}
