#pragma once
#include "auto_vk_toolkit.hpp"

#include "animation.hpp"
#include "lightsource_gpu_data.hpp"
#include "material_gpu_data.hpp"
#include "orca_scene.hpp"

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

/** @brief Compatible cache file version of the serializer
 *
 *  The version is the first value written to any cache file if the serializer is initialized in `serialize`-mode and
 *  the first value read from any cache file if the serializer is initialized in `deserialize`-mode. It is used to
 *  verfiy that the cache file's format is compatible with the serialization formats of the framework's state. If a
 *  framework function changes the format of the serialized/deserialized data, this version must be incremented to
 *  invalidate old cache files. An exception will be thrown if the cache file's version and the framework's serializer
 *  versions do not match.
 */
#define SERIALIZER_CACHE_FILE_VERSION 0x00000001

namespace avk {

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

	/** @brief serializer
	 *  
	 *  This type serializes/deserializes objects to/from binary files using the cereal
	 *  serialization library.
	 */
	class serializer
	{
	public:

		/** @brief The possible modes of the serializer
		 */
		enum class mode {
			serialize,
			deserialize
		};

		/** @brief Construct a serializer with serializing or deserializing capabilities
		 *
		 *  @param[in] aCacheFilePath The path to the cache file
		 *  @param[in] aMode serializer::mode::serialize for serialization
		 *					 serializer::mode::deserialize for deserialization
		 */
		serializer(std::string_view aCacheFilePath, serializer::mode aMode) :
			mArchive(aMode == serializer::mode::serialize ?
				std::variant<deserialize, serialize>{ serializer::serialize(aCacheFilePath) } :
				std::variant<deserialize, serialize>{ serializer::deserialize(aCacheFilePath) })
		{
			std::uint32_t version = SERIALIZER_CACHE_FILE_VERSION;
			archive(version);
			// If the mode is `deserialize`, version was overwritten by `archive` from the cache file and may be different
			if (version != SERIALIZER_CACHE_FILE_VERSION)
			{
				throw std::runtime_error("Versions of serializer and cache file do not match. Please delete the existing cache file and let it be recreated!");
			}
		}

		/** @brief Construct a serializer with serializing or deserializing capabilities
		 *  If the cache file from aCacheFilePath does not exists, the serializer is initialized
		 *  in serialization mode and creates the file for writing, else the serializer is
		 *  initialised in deserialization mode and reads from the file.
		 *
		 *  @param[in] aCacheFilePath The path to the cache file
		 */
		serializer(std::string_view aCacheFilePath) :
			serializer(aCacheFilePath, does_cache_file_exist(aCacheFilePath) ?
				serializer::mode::deserialize :
				serializer::mode::serialize)
		{ }

		serializer() = delete;
		serializer(serializer&&) noexcept = default;
		serializer(const serializer&) = delete;
		serializer& operator=(serializer&&) noexcept = default;
		serializer& operator=(const serializer&) = delete;
		~serializer() = default;

		/** @brief Returns the mode of the serializer
		 *
		 *  @param[out] The current mode, either serialize or deserialize
		 */
		mode mode() const
		{
			return std::holds_alternative<serialize>(mArchive) ? mode::serialize : mode::deserialize;
		}

		template<typename Type>
		using BinaryData = cereal::BinaryData<Type>;

		/** @brief Create binary data for const and non const pointers
		*   This function creates a wrapper around data, that can be binary serialized.
		* 
		*   @param[in] aValue Pointer to the beginning of the data
		*   @param[in] aSize The size of the data in size
		*   @return BinaryData structure
		*/
		template<typename Type>
		static BinaryData<Type> binary_data(Type&& aValue, size_t aSize)
		{
			return cereal::binary_data(std::forward<Type>(aValue), aSize);
		}

		/** @brief Serializes/Deserializes an Object
		 *
		 *  This function serializes the passed object if the serializer was initialized
		 *  in serialization mode and deserializes a file into the passed object if the
		 *	serializer was initialized in deserialization mode.
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
			if (mode() == mode::serialize) {
				std::get<serialize>(mArchive)(std::forward<Type>(aValue));
			}
			else {
				std::get<deserialize>(mArchive)(std::forward<Type>(aValue));
			}
		}

		/** @brief Serializes/Deserializes raw memory
		 *
		 *  This function serializes a block of memory of a specific size if the serializer
		 *  was initialized in serialization mode and deserializes the same size of memory
		 *  from file to the location of the pointer passed to this function if the
		 *  serializer was initialized in deserialization mode.
		 *
		 *  @param[in] aValue A pointer to the block of memory to serialize or to fill from file
		 *  @param[in] aSize The total size of the data in memory
		 */
		template<typename Type>
		inline void archive_memory(Type&& aValue, size_t aSize)
		{
			if (mode() == mode::serialize) {
				std::get<serialize>(mArchive)(binary_data(aValue, aSize));
			}
			else {
				std::get<deserialize>(mArchive)(binary_data(aValue, aSize));
			}
		}

		/** @brief Serializes/Deserializes a avk::buffer
		 *
		 *  This function serializes the buffer of the internal memory_handle if the serializer
		 *  was initialized in serialization mode and deserializes the buffer content from file
		 *  to the buffer of the internal memory_handle if the serializer was initialized in
		 *  deserialization mode. The passed avk::buffer is internally mapped and unmapped for
		 *  this operations.
		 *
		 *  @param[in] aValue A pointer to the block of memory to serialize or to fill from file
		 */
		inline void archive_buffer(avk::buffer_t& aValue)
		{
			size_t size = aValue.create_info().size;
			auto mapping =
				(mode() == mode::serialize) ?
				aValue.map_memory(avk::mapping_access::read) :
				aValue.map_memory(avk::mapping_access::write);
			archive_memory(mapping.get(), size);
		}

		/** @brief Flush the underlying output stream
		 *
		 *  This function can be used to explicitely flush the underlying outputstream during
		 *  serialization and requests all data to be written. During deserialization, this
		 *  function does nothing
		 */
		void flush()
		{
			if (mode() == mode::serialize) {
				std::get<serialize>(mArchive).flush();
			}
		}

	private:

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
			 *  @param[in] aCacheFilePath The filename including the full path where to save the cached file
			 */
			serialize(const std::string_view aCacheFilePath) :
				mOfstream(aCacheFilePath.data(), std::ios::binary),
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

			/** @brief Flush the underlying output stream
			 *
			 *  This function can be used to explicitely flush the underlying outputstream
			 *  and requests all data to be written.
			 */
			void flush()
			{
				mOfstream.flush();
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
			 *  @param[in] aCacheFilePath The filename including the full path to the binary cached file
			 */
			deserialize(const std::string_view aCacheFilePath) :
				mIfstream(aCacheFilePath.data(), std::ios::binary),
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

		std::variant<deserialize, serialize> mArchive;
	};
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

	template<typename Archive>
	void serialize(Archive& aArchive, glm::mat3& aValue)
	{
		aArchive(aValue[0], aValue[1], aValue[2]);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::mat4& aValue)
	{
		aArchive(aValue[0], aValue[1], aValue[2], aValue[3]);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, glm::quat& aValue)
	{
		aArchive(aValue.x, aValue.y, aValue.z, aValue.w);
	}
}

namespace avk {
	template<typename Archive>
	void serialize(Archive& aArchive, avk::model_instance_data& aValue)
	{
		aArchive(aValue.mName, aValue.mTranslation, aValue.mScaling, aValue.mRotation);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::material_gpu_data& aValue)
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
	void serialize(Archive& aArchive, avk::lightsource_gpu_data& aValue)
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

	template<typename Archive>
	void serialize(Archive& aArchive, avk::animation_clip_data& aValue)
	{
		aArchive(
			aValue.mAnimationIndex,
			aValue.mTicksPerSecond,
			aValue.mStartTicks,
			aValue.mEndTicks
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::position_key& aValue)
	{
		aArchive(
			aValue.mTime,
			aValue.mValue
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::rotation_key& aValue)
	{
		aArchive(
			aValue.mTime,
			aValue.mValue
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::scaling_key& aValue)
	{
		aArchive(
			aValue.mTime,
			aValue.mValue
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::mesh_bone_info& aValue)
	{
		aArchive(
			aValue.mMeshAnimationIndex,
			aValue.mMeshIndexInModel,
			aValue.mMeshLocalBoneIndex,
			aValue.mGlobalBoneIndexOffset
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::bone_mesh_data& aValue)
	{
		aArchive(
			aValue.mInverseBindPoseMatrix,
			aValue.mInverseMeshRootMatrix,
			aValue.mMeshBoneInfo
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::animated_node& aValue)
	{
		aArchive(
			aValue.mPositionKeys,
			aValue.mRotationKeys,
			aValue.mScalingKeys,
			aValue.mSameRotationAndPositionKeyTimes,
			aValue.mSameScalingAndPositionKeyTimes,
			aValue.mLocalTransform,
			aValue.mGlobalTransform,
			aValue.mAnimatedParentIndex,
			aValue.mParentTransform,
			aValue.mBoneMeshTargets
		);
	}

	template<typename Archive>
	void serialize(Archive& aArchive, avk::animation& aValue)
	{
		aArchive(
			aValue.mAnimationData,
			aValue.mAnimationIndex,
			aValue.mMaxNumBoneMatrices
		);
	}
}

namespace vk {
	template<typename Archive>
	void serialize(Archive& aArchive, vk::Extent3D& aValue)
	{
		aArchive(aValue.width, aValue.height, aValue.depth);
	}
}
