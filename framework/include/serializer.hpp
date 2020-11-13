#pragma once
#include "gvk.hpp"
/** cereal binary archive */
#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"

/** cereal stdlib type support */
#include "cereal/types/array.hpp"
#include "cereal/types/atomic.hpp"
#include "cereal/types/base_class.hpp"
#include "cereal/types/bitset.hpp"
#include "cereal/types/chrono.hpp"
#include "cereal/types/common.hpp"
#include "cereal/types/complex.hpp"
#include "cereal/types/deque.hpp"
#include "cereal/types/forward_list.hpp"
#include "cereal/types/functional.hpp"
#include "cereal/types/list.hpp"
#include "cereal/types/map.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/types/optional.hpp"
#include "cereal/types/polymorphic.hpp"
#include "cereal/types/queue.hpp"
#include "cereal/types/set.hpp"
#include "cereal/types/stack.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/tuple.hpp"
#include "cereal/types/unordered_map.hpp"
#include "cereal/types/unordered_set.hpp"
#include "cereal/types/utility.hpp"
#include "cereal/types/valarray.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/vector.hpp"

namespace gvk {

	/** @brief Serializer
	 *  
	 *  This type serializes/deserializes objects to/from binary files using the cereal
	 *  serialization library. The serializer must be initialised either with a
	 *  serializer::serialize or serializer::deserialize object to handle serialization.
	 */
	class serializer
	{
	public:

		class serialize {
			std::ofstream mOfstream;
			cereal::BinaryOutputArchive mArchive;

		public:
			serialize(const std::string& aPath) :
				mOfstream(aPath, std::ios::binary),
				mArchive(mOfstream)
			{}

			serialize(serialize&& aOther) noexcept :
				mOfstream(std::move(aOther.mOfstream)),
				mArchive(mOfstream)
			{}

			template<typename Type>
			void operator()(Type&& aValue)
			{
				mArchive(std::forward<Type>(aValue));
			}
		};

		class deserialize
		{
			std::ifstream mIfstream;
			cereal::BinaryInputArchive mArchive;

		public:
			deserialize(const std::string& aPath) :
				mIfstream(aPath, std::ios::binary),
				mArchive(mIfstream)
			{}

			deserialize(deserialize&& aOther) noexcept :
				mIfstream(std::move(aOther.mIfstream)),
				mArchive(mIfstream)
			{}

			template<typename Type>
			void operator()(Type&& aValue)
			{
				mArchive(std::forward<Type>(aValue));
			}
		};

		serializer(serialize&& aMode) :
			mArchive(std::move(aMode)),
			mMode(mode::serialize)
		{}

		serializer(deserialize&& aMode) :
			mArchive(std::move(aMode)),
			mMode(mode::deserialize)
		{}

		enum class mode {
			serialize,
			deserialize,
			none
		};

		const mode mode() const { return mMode; }

		/** @brief Serializes/Deserializes an Object
		 *
		 *  This function serializes the passed object if the serializer was initialized
		 *  with a serializer::serialize object and deserializes a file into the passed
		 *  object if the serializer was initialized with a serializer::deserialize object.
		 *
		 *  @param[in] aValue The object to serialize or to fill from the file
		 *
		 *  @errors A compiler error such as "cereal could not find any output serialization
		 *  functions * for the provided type and archive combination." means either a custom
		 *  serialization * function must be implemented (see Custom Serialization Functions
		 *  below) or the * custom serialization function is not defined in the same namespace
		 *  as the type to serialize.
		 */
		template<typename Type>
		inline void archive(Type&& aValue)
		{
			if (mMode == mode::serialize) {
				std::get<serialize>(mArchive)(std::forward<Type>(aValue));
			}
			else {
				std::get<deserialize>(mArchive)(std::forward<Type>(aValue));
			}
		}

		template<typename Type>
		using BinaryData = cereal::BinaryData<Type>;

		/** @brief Create binary data for const and non const pointers
		*   This function creates a wrapper around data, that can be
		*   binary serialized.
		* 
		*   @param[in] aValue Pointer to the beginning of the data
		*   @param[in] aSize The size of the data in size
		*   @return BinaryData structure
		*/
		template<typename Type>
		BinaryData<Type> binary_data(Type&& aValue, size_t aSize)
		{
			return cereal::binary_data(std::forward<Type>(aValue), aSize);
		}

	private:
		std::variant<deserialize, serialize> mArchive;
		enum class mode mMode;
	};

	static inline bool does_cache_file_exist(const std::string& aPath)
	{
		return std::filesystem::exists(aPath);
	}
}

/** @brief Custom Serialization Functions
 *
 *  For every custom type, not in stdlib, a custom serialize function has to
 *  be provided. If the type only has public members, external functions like
 *  below can be used without altering the type implementation. For private,
 *  internal and other variations see
 *  (http://uscilab.github.io/cereal/serialization_functions.html). The
 *  external function template has to be in the same namespace as the type
 *  for cereal to find it.
 *  
 *  External function template:
 *  
 *  template<typename Archive>
 *  void serialize(Archive& aArchive, YOUR_TYPE& aValue)
 *  {
 *  	aArchive(aValue.member1, aValue.member2, etc);
 *  }
 */

namespace glm {
	template<typename Archive>
	void serialize(Archive& aArchive, glm::vec2& aValue)
	{
		aArchive(aValue.x, aValue.y);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::vec3& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::vec4& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z, aValue.w);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::uvec4& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z, aValue.w);
	}
}

namespace avk {
	namespace cfg {
		template<typename Archive>
		void serialize(Archive& aArchive, avk::cfg::color_blending_config& aValue)
		{
			aArchive(aValue.mTargetAttachment);
			aArchive(aValue.mEnabled);
			aArchive(aValue.mAffectedColorChannels);
			aArchive(aValue.mIncomingColorFactor);
			aArchive(aValue.mExistingColorFactor);
			aArchive(aValue.mColorOperation);
			aArchive(aValue.mIncomingAlphaFactor);
			aArchive(aValue.mExistingAlphaFactor);
			aArchive(aValue.mAlphaOperation);
		};
	}
}

namespace gvk {
	template<typename Archive>
	void serialize(Archive& aArchive, gvk::model_instance_data& aValue)
	{
		aArchive(aValue.mName, aValue.mTranslation, aValue.mScaling, aValue.mRotation);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, gvk::material_config& aValue)
	{
		aArchive(aValue.mName);

		aArchive(aValue.mIgnoreCpuOnlyDataForEquality, aValue.mShadingModel, aValue.mWireframeMode, aValue.mTwosided, aValue.mBlendMode);

		aArchive(aValue.mDiffuseReflectivity, aValue.mAmbientReflectivity, aValue.mSpecularReflectivity,
			aValue.mEmissiveColor, aValue.mTransparentColor, aValue.mReflectiveColor, aValue.mAlbedo);

		aArchive(aValue.mOpacity, aValue.mBumpScaling, aValue.mShininess, aValue.mShininessStrength);

		aArchive(aValue.mRefractionIndex, aValue.mReflectivity, aValue.mMetallic, aValue.mSmoothness);

		aArchive(aValue.mSheen, aValue.mThickness, aValue.mRoughness, aValue.mAnisotropy);

		aArchive(aValue.mAnisotropyRotation, aValue.mCustomData);

		aArchive(aValue.mDiffuseTex);
		aArchive(aValue.mSpecularTex);
		aArchive(aValue.mAmbientTex);
		aArchive(aValue.mEmissiveTex);
		aArchive(aValue.mHeightTex);
		aArchive(aValue.mNormalsTex);
		aArchive(aValue.mShininessTex);
		aArchive(aValue.mOpacityTex);
		aArchive(aValue.mDisplacementTex);
		aArchive(aValue.mReflectionTex);
		aArchive(aValue.mLightmapTex);
		aArchive(aValue.mExtraTex);

		aArchive(aValue.mDiffuseTexOffsetTiling);
		aArchive(aValue.mSpecularTexOffsetTiling);
		aArchive(aValue.mAmbientTexOffsetTiling);
		aArchive(aValue.mEmissiveTexOffsetTiling);
		aArchive(aValue.mHeightTexOffsetTiling);
		aArchive(aValue.mNormalsTexOffsetTiling);
		aArchive(aValue.mShininessTexOffsetTiling);
		aArchive(aValue.mOpacityTexOffsetTiling);
		aArchive(aValue.mDisplacementTexOffsetTiling);
		aArchive(aValue.mReflectionTexOffsetTiling);
		aArchive(aValue.mLightmapTexOffsetTiling);
		aArchive(aValue.mExtraTexOffsetTiling);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, gvk::material_gpu_data& aValue)
	{
		aArchive(
			aValue.mDiffuseReflectivity,
			aValue.mAmbientReflectivity,
			aValue.mSpecularReflectivity,
			aValue.mEmissiveColor,
			aValue.mTransparentColor,
			aValue.mReflectiveColor,
			aValue.mAlbedo
		);

		aArchive(
			aValue.mOpacity,
			aValue.mBumpScaling,
			aValue.mShininess,
			aValue.mShininessStrength,
			aValue.mRefractionIndex,
			aValue.mReflectivity,
			aValue.mMetallic,
			aValue.mSmoothness,
			aValue.mSheen,
			aValue.mThickness,
			aValue.mRoughness,
			aValue.mAnisotropy
		);

		aArchive(
			aValue.mAnisotropyRotation,
			aValue.mCustomData
		);

		aArchive(
			aValue.mDiffuseTexIndex,
			aValue.mSpecularTexIndex,
			aValue.mAmbientTexIndex,
			aValue.mEmissiveTexIndex,
			aValue.mHeightTexIndex,
			aValue.mNormalsTexIndex,
			aValue.mShininessTexIndex,
			aValue.mOpacityTexIndex,
			aValue.mDisplacementTexIndex,
			aValue.mReflectionTexIndex,
			aValue.mLightmapTexIndex,
			aValue.mExtraTexIndex
		);

		aArchive(
			aValue.mDiffuseTexOffsetTiling,
			aValue.mSpecularTexOffsetTiling,
			aValue.mAmbientTexOffsetTiling,
			aValue.mEmissiveTexOffsetTiling,
			aValue.mHeightTexOffsetTiling,
			aValue.mNormalsTexOffsetTiling,
			aValue.mShininessTexOffsetTiling,
			aValue.mOpacityTexOffsetTiling,
			aValue.mDisplacementTexOffsetTiling,
			aValue.mReflectionTexOffsetTiling,
			aValue.mLightmapTexOffsetTiling,
			aValue.mExtraTexOffsetTiling
		);
	}
}
