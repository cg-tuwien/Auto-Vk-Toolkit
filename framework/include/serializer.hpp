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

	/** @brief serializer
	 *  
	 *  This type serializes/deserializes objects to/from binary files using the cereal
	 *  serialization library. The serializer must be initialised either with a
	 *  serializer::serialize or serializer::deserialize object to handle serialization.
	 */
	class serializer
	{
	public:

		/** @brief serialize
		 *
		 *  This type represents an output archive to save data in binary form to a file.
		 */
		class serialize {
			std::ofstream mOfstream;
			cereal::BinaryOutputArchive mArchive;

		public:
			serialize() = delete;

			/** @brief Construct, outputting a binary file to the provided path
			 *
			 *  @param[in] aFile The filename including the full path where to save the cached file
			 */
			serialize(const std::string& aFile) :
				mOfstream(aFile, std::ios::binary),
				mArchive(mOfstream)
			{}

			/* Construct from other serialize */
			serialize(serialize&& aOther) noexcept :
				mOfstream(std::move(aOther.mOfstream)),
				mArchive(mOfstream)
			{}

			serialize(const serialize&) = delete;
			serialize& operator=(serialize&&) noexcept = default;
			serialize& operator=(const serialize&) = delete;
			~serialize() = default;

			/** @brief Serializes an Object
			 *
			 *  This function serializes the passed object to a binary file
			 *
			 *  @param[in] aValue The object to serialize
			 */
			template<typename Type>
			void operator()(Type&& aValue)
			{
				mArchive(std::forward<Type>(aValue));
			}
		};

		/** @brief deserialize
		 *
		 *  This type represents an input archive to retrieve data in binary form from a file.
		 */
		class deserialize
		{
			std::ifstream mIfstream;
			cereal::BinaryInputArchive mArchive;

		public:
			deserialize() = delete;

			/** @brief Construct, reading a binary file from the provided file
			 *
			 *  @param[in] aFile The filename including the full path to the binary cached file
			 */
			deserialize(const std::string& aFile) :
				mIfstream(aFile, std::ios::binary),
				mArchive(mIfstream)
			{}

			/* Construct from other deserialize */
			deserialize(deserialize&& aOther) noexcept :
				mIfstream(std::move(aOther.mIfstream)),
				mArchive(mIfstream)
			{}

			deserialize(const deserialize&) = delete;
			deserialize& operator=(deserialize&&) noexcept = default;
			deserialize& operator=(const deserialize&) = delete;
			~deserialize() = default;

			/** @brief Deserializes an Object
			 *
			 *  This function deserializes the object from a binary file.
			 *
			 *  @param[in] aValue The object to fill from file
			 */
			template<typename Type>
			void operator()(Type&& aValue)
			{
				mArchive(std::forward<Type>(aValue));
			}
		};

		serializer() = delete;

		/** @brief Construct a serializer with serializing capabilities
		 *
		 *  @param[in] aMode A serialize object
		 */
		serializer(serialize&& aMode) noexcept :
			mArchive(std::move(aMode)),
			mMode(mode::serialize)
		{}

		/** @brief Construct a serializer with deserializing capabilities
		 *
		 *  @param[in] aMode A deserialize object
		 */
		serializer(deserialize&& aMode) noexcept :
			mArchive(std::move(aMode)),
			mMode(mode::deserialize)
		{}

		serializer(serializer&&) noexcept = default;
		serializer(const serializer&) = delete;
		serializer& operator=(serializer&&) noexcept = default;
		serializer& operator=(const serializer&) = delete;
		~serializer() = default;

		/** @brief The possible modes of the serializer
		 */
		enum class mode {
			serialize,
			deserialize
		};

		/** @brief Returns the mode of the serializer
		 *
		 *  @param[out] The current mode, either serialize or deserialize
		 */
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
		inline void archive_buffer(avk::resource_reference<avk::buffer_t> aValue)
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

	/** @brief Checks if a cache file exists
	 *
	 *  @param[in] aPath The path to a cached file
	 *
	 *  @param[out] True if the cache file exists, false otherwise
	 */
	static inline bool does_cache_file_exist(const std::string_view aPath)
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
