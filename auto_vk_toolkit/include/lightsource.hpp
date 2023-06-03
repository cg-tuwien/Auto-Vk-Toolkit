#pragma once

namespace avk
{
	/** Type of a light source */
	enum struct lightsource_type {
		ambient			= 0,
		directional		= 1,
		point			= 2,
		spot			= 3,
		area			= 4,
		reserved0		= 5, 
		reserved1		= 6, 
		reserved2		= 7, 
		reserved3		= 8, 
		reserved4		= 9, 
		custom0			= 10, 
		custom1			= 11, 
		custom2			= 12, 
		custom3			= 13, 
		custom4			= 14, 
		custom5			= 15 
	};

	/**	Struct containing data about a specific light source
	 */
	struct lightsource
	{
		std::string mName;
		lightsource_type mType;
		glm::vec3 mColor; // Contains what Assimp returns as the "diffuse color"
		glm::vec3 mDirection;
		glm::vec3 mPosition;

		float mAngleOuterCone;
		float mAngleInnerCone;
		float mFalloff;
		
		float mAttenuationConstant;
		float mAttenuationLinear;
		float mAttenuationQuadratic;

		// Assimp returns the following data, but a.t.m. they are not considered for avk::lightsource_gpu_data.
		glm::vec3 mColorAmbient;
		glm::vec3 mColorSpecular;
		glm::vec3 mUpVector;
		glm::vec2 mAreaExtent;

		static lightsource create_ambient(glm::vec3 aColor = glm::vec3{0.1f}, std::string aName = "")
		{
			return lightsource {
				std::move(aName),
				lightsource_type::ambient,
				aColor,
				{},
				{},
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec2{0.0f}
			};
		}
		
		static lightsource create_directional(glm::vec3 aDirection, glm::vec3 aColor = glm::vec3{0.1f}, std::string aName = "")
		{
			return lightsource {
				std::move(aName),
				lightsource_type::directional,
				aColor,
				aDirection,
				{},
				0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec2{0.0f}
			};
		}
		
		static lightsource create_pointlight(glm::vec3 aPosition, glm::vec3 aColor = glm::vec3{0.1f}, std::string aName = "")
		{
			return lightsource {
				std::move(aName),
				lightsource_type::point,
				aColor,
				{},
				aPosition,
				0.0f, 0.0f, 0.0f,
				1.0f, 0.1f, 0.01f,
				glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec2{0.0f}
			};
		}
		
		static lightsource create_spotlight(glm::vec3 aPosition, glm::vec3 aDirection, float aOuterConeAngle = glm::half_pi<float>(), float aInnerConeAngle = glm::quarter_pi<float>(), float aFalloff = 1.0f, glm::vec3 aColor = glm::vec3{0.1f}, std::string aName = "")
		{
			return lightsource {
				std::move(aName),
				lightsource_type::spot,
				aColor,
				aDirection,
				aPosition,
				aOuterConeAngle, aInnerConeAngle, aFalloff,
				1.0f, 0.1f, 0.01f,
				glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec3{0.0f}, glm::vec2{0.0f}
			};
		}

		lightsource& set_attenuation(float aConstAttenuation, float aLinearAttenuation, float aQuadraticAttenuation)
		{
			mAttenuationConstant = aConstAttenuation;
			mAttenuationLinear = aLinearAttenuation;
			mAttenuationQuadratic = aQuadraticAttenuation;
			return *this;
		}
	};

	/** Compare two light source data for equality. Only the light source settings are
	 *	used for comparison, the name is ignored. */
	static bool operator ==(const lightsource& left, const lightsource& right) {
		if (left.mAngleInnerCone != right.mAngleInnerCone) return false;
		if (left.mAngleOuterCone != right.mAngleOuterCone) return false;
		if (left.mAttenuationConstant != right.mAttenuationConstant) return false;
		if (left.mAttenuationLinear != right.mAttenuationLinear) return false;
		if (left.mAttenuationQuadratic != right.mAttenuationQuadratic) return false;
		if (left.mColor != right.mColor) return false;
		if (left.mDirection != right.mDirection) return false;
		if (left.mPosition != right.mPosition) return false;
		if (left.mName != right.mName) return false;
		if (left.mType != right.mType) return false;
		if (left.mColorAmbient != right.mColorAmbient) return false;
		if (left.mColorSpecular != right.mColorSpecular) return false;
		if (left.mUpVector != right.mUpVector) return false;
		if (left.mAreaExtent != right.mAreaExtent) return false;
		return true;
	}

	/** Compare two light source data for inequality. Only the light source settings are
	 *	used for comparison, the name is ignored. */
	static bool operator !=(const lightsource& left, const lightsource& right) {
		return !(left == right);
	}
}

namespace std
{
	template<> struct hash<avk::lightsource>
	{
		std::size_t operator()(avk::lightsource const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, 
				o.mAngleInnerCone,
				o.mAngleOuterCone,
				o.mAttenuationConstant,
				o.mAttenuationLinear,
				o.mAttenuationQuadratic,
				o.mColor,
				o.mDirection,
				o.mPosition,
				o.mType,
				o.mColorAmbient,
				o.mColorSpecular,
				o.mUpVector,
				o.mAreaExtent
			);
			return h;
		}
	};
}