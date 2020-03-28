#pragma once

namespace cgb
{
	/** Light data in the right format to be uploaded to the GPU
	 *	and to be used in a GPU buffer like a UBO or an SSBO
	 */
	struct lightsource_gpu_data
	{
		/** Ambient color of the light source. */
		alignas(16) glm::vec4 mColorAmbient;
		/** Diffuse color of the light source. */
		alignas(16) glm::vec4 mColorDiffuse;
		/** Specular color of the light source. */
		alignas(16) glm::vec4 mColorSpecular;
		/** Direction of the light source. */
		alignas(16) glm::vec4 mDirection;
		/** Position of the light source. */
		alignas(16) glm::vec4 mPosition;
		/** Angles, where the individual elements contain the following data:
		 *  [0] ... inner cone angle
		 *  [1] ... outer cone angle
		 *  [2] ... unused
		 *  [3] ... unused
		 */
		alignas(16) glm::vec4 mAngles;
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
		if (left.mColorAmbient != right.mColorAmbient) return false;
		if (left.mColorDiffuse != right.mColorDiffuse) return false;
		if (left.mColorSpecular != right.mColorSpecular) return false;
		if (left.mDirection != right.mDirection) return false;
		if (left.mPosition != right.mPosition) return false;
		if (glm::vec3(left.mAttenuation) != glm::vec3(right.mAttenuation)) return false;
		if (glm::vec2(left.mAngles) != glm::vec2(right.mAngles)) return false;
		if (left.mInfo[0] != right.mInfo[0]) return false;
		return true;
	}

	// Compare two `lightsource_gpu_data` structs for inequality, by comparing them element-wise
	static bool operator !=(const lightsource_gpu_data& left, const lightsource_gpu_data& right) {
		return !(left == right);
	}

	// Convert the given lightsource data into a format which is suitable for GPU buffer usage
	std::vector<lightsource_gpu_data> convert_lights_for_gpu_usage(std::vector<lightsource> _LightSources);
}

namespace std
{
	template<> struct hash<cgb::lightsource_gpu_data>
	{
		std::size_t operator()(cgb::lightsource_gpu_data const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h, 
				o.mColorAmbient,
				o.mColorDiffuse,
				o.mColorSpecular,
				o.mDirection,
				o.mPosition,
				glm::vec2(o.mAngles),
				glm::vec3(o.mAttenuation),
				o.mInfo[0]
			);
			return h;
		}
	};
}
