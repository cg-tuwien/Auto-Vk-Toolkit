#pragma once

namespace avk
{
	/** Light data in the right format to be uploaded to the GPU
	 *	and to be used in a GPU buffer like a UBO or an SSBO
	 */
	struct lightsource_gpu_data
	{
		/** Color of the light source. */
		alignas(16) glm::vec4 mColor;

		/** Direction of the light source. */
		alignas(16) glm::vec4 mDirection;

		/** Position of the light source. */
		alignas(16) glm::vec4 mPosition;

		/** Angles, where the individual elements contain the following data:
		 *  [0] ... cosine of halve outer cone angle, i.e. cos(outer/2)
		 *  [1] ... cosine of halve inner cone angle, i.e. cos(inner/2)
		 *  [2] ... falloff
		 *  [3] ... unused
		 */
		alignas(16) glm::vec4 mAnglesFalloff;

		/** Light source attenuation, where the individual elements contain the following data:
		 *  [0] ... constant attenuation factor
		 *  [1] ... linear attenuation factor
		 *  [2] ... quadratic attenuation factor
		 *  [3] ... unused
		 */
		alignas(16) glm::vec4 mAttenuation;

		/** General information about the light source, where the individual elements contain the following data:
		 *  [0] ... type of the light source
		 */
		alignas(16) glm::ivec4 mInfo;
	};

	// Compare two `lightsource_gpu_data` structs for equality, by comparing them element-wise
	static bool operator ==(const lightsource_gpu_data& left, const lightsource_gpu_data& right) {
		if (left.mColor != right.mColor) return false;
		if (left.mDirection != right.mDirection) return false;
		if (left.mPosition != right.mPosition) return false;
		if (glm::vec3(left.mAnglesFalloff) != glm::vec3(right.mAnglesFalloff)) return false;
		if (glm::vec3(left.mAttenuation) != glm::vec3(right.mAttenuation)) return false;
		if (left.mInfo[0] != right.mInfo[0]) return false;
		return true;
	}

	// Compare two `lightsource_gpu_data` structs for inequality, by comparing them element-wise
	static bool operator !=(const lightsource_gpu_data& left, const lightsource_gpu_data& right) {
		return !(left == right);
	}

	/** Convert the given lightsource data into a format which is suitable for GPU buffer usage
	 *	@param	aLightsourceData		The input, i.e. the collection of `avk::lightsource` elements to be converted into `avk::lightsource_gpu_data` elements.
	 *	@param	aNumElements			The number of input elements to be converted.
	 *	@param	aTransformationMatrix	A matrix which can be used to transform into a specific space.
	 *	@param	aDestination			Reference to the output vector which must be some collection of `avk::lightsource_gpu_data` that is accessible via
	 *									index operator and must already be pre-allocated to support access up to [aNumElements-1].
	 */
	template <typename Out, typename In> requires avk::has_subscript_operator<Out> && avk::has_subscript_operator<In>
	void convert_for_gpu_usage(const In& aLightsourceData, const size_t aNumElements, glm::mat4 aTransformationMatrix, Out& aDestination)
	{
		const auto aInvTranspMat3 = glm::mat3(glm::inverse(glm::transpose(aTransformationMatrix)));
		for (size_t i = 0; i < aNumElements; ++i) {
			aDestination[i].mColor				= glm::vec4(aLightsourceData[i].mColor, 0.0f);
			aDestination[i].mDirection			= glm::vec4(aInvTranspMat3 * aLightsourceData[i].mDirection, 0.0f);
			aDestination[i].mPosition			= aTransformationMatrix * glm::vec4(aLightsourceData[i].mPosition, 1.0f);
			aDestination[i].mAnglesFalloff[0]	= glm::cos(aLightsourceData[i].mAngleOuterCone * 0.5f);
			aDestination[i].mAnglesFalloff[1]	= glm::cos(aLightsourceData[i].mAngleInnerCone * 0.5f);
			aDestination[i].mAnglesFalloff[2]	= aLightsourceData[i].mFalloff;
			aDestination[i].mAttenuation[0]		= aLightsourceData[i].mAttenuationConstant;
			aDestination[i].mAttenuation[1]		= aLightsourceData[i].mAttenuationLinear;
			aDestination[i].mAttenuation[2]		= aLightsourceData[i].mAttenuationQuadratic;
			aDestination[i].mInfo[0]			= static_cast<int>(aLightsourceData[i].mType);
		}
	}
	
	/** Convert the given lightsource data into a format which is suitable for GPU buffer usage
	 *	@param	aLightsourceData		The input, i.e. the collection of `avk::lightsource` elements to be converted into `avk::lightsource_gpu_data` elements.
	 *	@param	aNumElements			The number of input elements to be converted.
	 *	@param	aTransformationMatrix	A matrix which can be used to transform into a specific space.
	 *	@tparam	In						Input types must provide data access through subscript operators.
	 *	@tparam	Out						Specify the type of the result collection, e.g. like follows: `auto result = avk::convert_for_gpu_usage<std::array<avk::lightsource_gpu_data, 10>>(...)`
	 */
	template <typename Out, typename In>
	Out convert_for_gpu_usage(const In& aLightsourceData, const size_t aNumElements, glm::mat4 aTransformationMatrix = glm::mat4{1.0f})
	{
		Out gpuLights{};
		convert_for_gpu_usage(aLightsourceData, aNumElements, aTransformationMatrix, gpuLights);
		return gpuLights;
	}

	/** Convert the given lightsource data into a format which is suitable for GPU buffer usage
	 *	@param	aLightsourceData		The input, i.e. the collection of `avk::lightsource` elements to be converted into `avk::lightsource_gpu_data` elements.
	 *	@param	aNumElements			The number of input elements to be converted.
	 *	@param	aTransformationMatrix	A matrix which can be used to transform into a specific space.
	 *	@tparam	In						This overload is invoked if type `In::resize(size_t)` exists. Furthermore, subscript operators must be provided. Most notably, this applies to `std::vector`.
	 *	@tparam	Out						Specify the type of the result collection, where in using this overload, the resulting collection will be resized
	 *									to a size of `aNumElements`. Example usage: `auto result = avk::convert_for_gpu_usage<std::vector<avk::lightsource_gpu_data>>(...)`
	 */
	template <typename Out, typename In> requires avk::has_resize<Out>
	Out convert_for_gpu_usage(const In& aLightsourceData, const size_t aNumElements, glm::mat4 aTransformationMatrix = glm::mat4{1.0f})
	{
		Out gpuLights{};
		gpuLights.resize(aNumElements);
		convert_for_gpu_usage(aLightsourceData, aNumElements, aTransformationMatrix, gpuLights);
		return gpuLights;
	}

	/** Convenience overload on top of the other overloads that automatically determines the number of elements based on `aLightsourceData`.
	 *	@param	aLightsourceData		The input, i.e. the collection of `avk::lightsource` elements to be converted into `avk::lightsource_gpu_data` elements.
	 *	@param	aTransformationMatrix	A matrix which can be used to transform into a specific space.
	 *	@tparam	In						Input types must provide data access through subscript operators.
	 *	@tparam Out						May be a type that has a `.resize()` member or that doesn't. Either is fine and the respective overload will be invoked.
	 */
	template <typename Out, typename In>
	std::enable_if_t<avk::has_size_and_iterators<In>::value, Out> convert_for_gpu_usage(const In& aLightsourceData, glm::mat4 aTransformationMatrix = glm::mat4{1.0f})
	{
		return convert_for_gpu_usage<Out, In>(aLightsourceData, aLightsourceData.size(), aTransformationMatrix);
	}

	
}

namespace std
{
	template<> struct hash<avk::lightsource_gpu_data>
	{
		std::size_t operator()(avk::lightsource_gpu_data const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, 
				o.mColor,
				o.mDirection,
				o.mPosition,
				glm::vec3(o.mAttenuation),
				glm::vec3(o.mAnglesFalloff),
				o.mInfo[0]
			);
			return h;
		}
	};
}
