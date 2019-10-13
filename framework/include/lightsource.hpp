#pragma once

namespace cgb
{
	/**	Lightsource struct which hold all the concrete data,
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
		int mType;
	};

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