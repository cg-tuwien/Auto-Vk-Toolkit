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
			deserialize
		};

		const mode mode() const { return mMode; }

		/** @brief Serializes/Deserializes an Object
		 *
		 *  This function serializes the passed object if the serializer was initialized
		 *  with a serializer::serialize object and deserializes a file into the passed
		 *  object if the serializer was initialized with a serializer::deserialize object.
		 *
		 *  @param[in] aValue The object to serialize or to fill from file
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

		/** @brief Serializes/Deserializes raw memory
		 *
		 *  This function serializes a block of memory of a specific size if the serializer
		 *  was initialized with a serializer::serialize object and deserializes the same size
		 *  of memory from file to the location of the pointer passed to this function if the
		 *  serializer was initialized with a serializer::deserialize object.
		 *
		 *  @param[in] aValue A pointer to the block of memory to serialize or to fill from file
		 *  @param[in] aSize The total size of the data in memory
		 */
		template<typename Type>
		inline void archive_memory(Type&& aValue, size_t aSize)
		{
			if (mMode == mode::serialize) {
				std::get<serialize>(mArchive)(cereal::binary_data(std::forward<Type>(aValue), aSize));
			}
			else {
				std::get<deserialize>(mArchive)(cereal::binary_data(std::forward<Type>(aValue), aSize));
			}
		}

		/** @brief Serializes/Deserializes a avk::buffer
		 *
		 *  This function serializes the buffer of the internal memory_handle if the serializer
		 *  was initialized with a serializer::serialize object and deserializes the buffer content
		 *  from file to the buffer of the internal memory_handle if the serializer was initialized with
		 *  a serializer::deserialize object. The passed avk::buffer is internally mapped and unmapped for
		 *  this operations.
		 *
		 *  @param[in] aValue A pointer to the block of memory to serialize or to fill from file
		 */
		inline void archive_buffer(avk::buffer& aValue)
		{
			size_t size = aValue.get().config().size;
			auto mapping =
				(mMode == mode::serialize) ?
				aValue->map_memory(avk::mapping_access::read) :
				aValue->map_memory(avk::mapping_access::write);
			archive_memory(mapping.get(), size);
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
	void serialize(Archive& aArchive, glm::uvec2& aValue)
	{
		aArchive(aValue.x, aValue.y);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::uvec3& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::uvec4& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z, aValue.w);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::ivec2& aValue)
	{
		aArchive(aValue.x, aValue.y);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::ivec3& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::ivec4& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z, aValue.w);
	}
}

namespace gvk {
	template<typename Archive>
	void serialize(Archive& aArchive, gvk::model_instance_data& aValue)
	{
		aArchive(aValue.mName, aValue.mTranslation, aValue.mScaling, aValue.mRotation);
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

	template<typename Archive>
	void serialize(Archive& aArchive, gvk::lightsource_gpu_data& aValue)
	{
		aArchive(
			aValue.mColor,
			aValue.mDirection,
			aValue.mPosition,
			aValue.mAnglesFalloff,
			aValue.mAttenuation,
			aValue.mInfo
		);
	}
}
