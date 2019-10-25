#pragma once

namespace cgb
{
	/** Type of a light source */
	enum struct lightsource_type {
		undefined		= 0,
		directional		= 1,
		point			= 2,
		spot			= 3, 
	};

	/**	Struct containing data about a specific light source
	 */
	struct lightsource
	{
		std::string mName;

		float mAngleInnerCone;
		float mAngleOuterCone;
		float mAttenuationConstant;
		float mAttenuationLinear;
		float mAttenuationQuadratic;
		glm::vec3 mColorAmbient;
		glm::vec3 mColorDiffuse;
		glm::vec3 mColorSpecular;
		glm::vec3 mDirection;
		glm::vec3 mPosition;
		lightsource_type mType;
	};

	/** Compare two light source data for equality. Only the lightsource settings are
	 *	used for comparison, the name is ignored. */
	static bool operator ==(const lightsource& left, const lightsource& right) {
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
		if (left.mName != right.mName) return false;
		if (left.mType != right.mType) return false;
		return true;
	}

	/** Compare two light source data for inequality. Only the lightsource settings are
	 *	used for comparison, the name is ignored. */
	static bool operator !=(const lightsource& left, const lightsource& right) {
		return !(left == right);
	}
}

namespace std
{
	template<> struct hash<cgb::lightsource>
	{
		std::size_t operator()(cgb::lightsource const& o) const noexcept
		{
			std::size_t h = 0;
			cgb::hash_combine(h, 
				o.mAngleInnerCone,
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