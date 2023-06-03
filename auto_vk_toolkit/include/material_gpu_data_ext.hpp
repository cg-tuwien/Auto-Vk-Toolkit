#pragma once

#include "material_gpu_data.hpp"

namespace avk
{
	/**	EXTENDED (by the UV-Set per texture, and texture coordinates rotation per texture)
	 *  material data in a format suitable for GPU upload,
	 *	and to be used in a GPU buffer like a UBO or an SSBO.
	 *
	 *	Possible corresponding GLSL data structures:
	 *
	 *	layout(set = 0, binding = 0) uniform sampler2D textures[];
	 *
	 *	struct MaterialGpuDataExt
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
	 *		
	 *		uint mDiffuseTexUvSet;
	 *		uint mSpecularTexUvSet;
	 *		uint mAmbientTexUvSet;
	 *		uint mEmissiveTexUvSet;
	 *		uint mHeightTexUvSet;
	 *		uint mNormalsTexUvSet;
	 *		uint mShininessTexUvSet;
	 *		uint mOpacityTexUvSet;
	 *		uint mDisplacementTexUvSet;
	 *		uint mReflectionTexUvSet;
	 *		uint mLightmapTexUvSet;
	 *		uint mExtraTexUvSet;
	 *
	 *		float mDiffuseTexRotation;
	 *		float mSpecularTexRotation;
	 *		float mAmbientTexRotation;
	 *		float mEmissiveTexRotation;
	 *		float mHeightTexRotation;
	 *		float mNormalsTexRotation;
	 *		float mShininessTexRotation;
	 *		float mOpacityTexRotation;
	 *		float mDisplacementTexRotation;
	 *		float mReflectionTexRotation;
	 *		float mLightmapTexRotation;
	 *		float mExtraTexRotation;
	 *	};
	 *
	 *	layout(set = 0, binding = 1) buffer Material
	 *	{
	 *		MaterialGpuDataExt materials[];
	 *	} materialsBuffer;
	 *
	 */
	struct material_gpu_data_ext : public material_gpu_data // use non-virtual inheritance!
	{
		alignas(4) uint32_t    mDiffuseTexUvSet;
		alignas(4) uint32_t    mSpecularTexUvSet;
		alignas(4) uint32_t    mAmbientTexUvSet;
		alignas(4) uint32_t    mEmissiveTexUvSet;
		alignas(4) uint32_t    mHeightTexUvSet;
		alignas(4) uint32_t    mNormalsTexUvSet;
		alignas(4) uint32_t    mShininessTexUvSet;
		alignas(4) uint32_t    mOpacityTexUvSet;
		alignas(4) uint32_t    mDisplacementTexUvSet;
		alignas(4) uint32_t    mReflectionTexUvSet;
		alignas(4) uint32_t    mLightmapTexUvSet;
		alignas(4) uint32_t    mExtraTexUvSet;

		alignas(4) float       mDiffuseTexRotation;
		alignas(4) float       mSpecularTexRotation;
		alignas(4) float       mAmbientTexRotation;
		alignas(4) float       mEmissiveTexRotation;
		alignas(4) float       mHeightTexRotation;
		alignas(4) float       mNormalsTexRotation;
		alignas(4) float       mShininessTexRotation;
		alignas(4) float       mOpacityTexRotation;
		alignas(4) float       mDisplacementTexRotation;
		alignas(4) float       mReflectionTexRotation;
		alignas(4) float       mLightmapTexRotation;
		alignas(4) float       mExtraTexRotation;
	};

	/** Compares the two `material_gpu_data_ext`s for equality.
	 */
	static bool operator ==(const material_gpu_data_ext& left, const material_gpu_data_ext& right)
	{
		if (static_cast<const material_gpu_data&>(left) != static_cast<const material_gpu_data&>(right)) return false;

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

		return true;
	}

	/** Negated result of operator==, see the equality operator for more details! */
	static bool operator !=(const material_gpu_data_ext& left, const material_gpu_data_ext& right)
	{
		return !(left == right);
	}
}

namespace std // Inject hash for `avk::material_gpu_data` into std::
{
	template<> struct hash<avk::material_gpu_data_ext>
	{
		std::size_t operator()(avk::material_gpu_data_ext const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h,
				static_cast<const avk::material_gpu_data&>(o),
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
			return h;
		}
	};
}
