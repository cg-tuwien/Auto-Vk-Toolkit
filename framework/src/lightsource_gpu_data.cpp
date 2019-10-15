#include <cg_base.hpp>

namespace cgb {
	std::vector<lightsource_gpu_data> cgb::convert_lights_for_gpu_usage(std::vector<lightsource> _LightSources)
	{
		std::vector<lightsource_gpu_data> gpuLights;
		gpuLights.reserve(_LightSources.size());

		for (auto& ls : _LightSources) {
			auto& gl = gpuLights.emplace_back();
			gl.mAngles[0] = ls.mAngleInnerCone;
			gl.mAngles[1] = ls.mAngleOuterCone;
			gl.mAttenuation[0] = ls.mAttenuationConstant;
			gl.mAttenuation[1] = ls.mAttenuationLinear;
			gl.mAttenuation[2] = ls.mAttenuationQuadratic;
			gl.mColorAmbient = glm::vec4(ls.mColorAmbient, 0.0f);
			gl.mColorDiffuse = glm::vec4(ls.mColorDiffuse, 0.0f);
			gl.mColorSpecular = glm::vec4(ls.mColorSpecular, 0.0f);
			gl.mDirection = glm::vec4(ls.mDirection, 0.0f);
			gl.mPosition = glm::vec4(ls.mPosition, 0.0f);
			gl.mInfo[0] = static_cast<int>(ls.mType);
		}
		return gpuLights;
	}

}