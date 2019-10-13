#pragma once

namespace cgb
{
	/** Light data in the right format to be uploaded to the GPU
	* and to be used in a GPU buffer like a UBO or an SSBO
	*/
	struct lightsource_gpu_data
	{
		alignas(4)  float mAngleInnerCone;
		alignas(4)  float mAngleOuterCone;
		alignas(4)  float mAttenuationConstant;
		alignas(4)  float mAttenuationLinear;
		alignas(4)  float mAttenuationQuadratic;
		alignas(16) glm::vec4 mColorAmbient;
		alignas(16) glm::vec4 mColorDiffuse;
		alignas(16) glm::vec4 mColorSpecular;
		alignas(16) glm::vec4 mDirection;
		alignas(16) glm::vec4 mPosition;
		alignas(4)  int mType;
	};

	static bool operator ==(const lightsource_gpu_data& left, const lightsource_gpu_data& right) {
		if (left.mAngleInnerCone != right.mAngleInnerCone) return false;
		if (left.mAngleOuterCone != right.mAngleOuterCone) return false;
		if (left.mAttenuationConstant != right.mAttenuationConstant) return false;
		if (left.mAttenuationLinear != right.mAttenuationLinear) return false;
		if (left.mAttenuationQuadratic != right.mAttenuationQuadratic) return false;
		if (left.mColorAmbient != right.mColorAmbient) return false;
		if (left.mColorDiffuse != right.mColorDiffuse) return false;
		if (left.mColorSpecular != right.mColorSpecular) return false;
		if (left.mDirection != right.mDirection) return false;
		if (left.mPosition != right.mPosition) return false;
		if (left.mType != right.mType) return false;
		return true;
	}

	static bool operator !=(const lightsource_gpu_data& left, const lightsource_gpu_data& right) {
		return !(left == right);
	}

	std::vector<lightsource_gpu_data> convert_lights_for_gpu_usage(std::vector <lightsource> _LightSources);
}

namespace std
{
	template<> struct hash<cgb::lightsource_gpu_data>
	{
		std::size_t operator()(cgb::lightsource_gpu_data const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h, o.mAngleInnerCone,
				o.mAngleOuterCone,
				o.mAttenuationConstant,
				o.mAttenuationLinear,
				o.mAttenuationQuadratic,
				o.mColorAmbient,
				o.mColorDiffuse,
				o.mColorSpecular,
				o.mDirection,
				o.mPosition,
				o.mType
			);
			return h;
		}
	};
}