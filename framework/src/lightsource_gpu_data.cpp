#include <cg_base.hpp>

namespace cgb {
	std::vector<lightsource_gpu_data> cgb::convert_lights_for_gpu_usage(std::vector<lightsource> _LightSources)
	{
		std::vector<lightsource_gpu_data> gpuLights;
		gpuLights.reserve(_LightSources.size());

		for (auto& ls : _LightSources) {
			auto& gl = gpuLights.emplace_back();
			gl.mAngleInnerCone = ls.mAngleInnerCone;
			gl.mAngleOuterCone = ls.mAngleOuterCone;
			gl.mAttenuationConstant = ls.mAttenuationConstant;
			gl.mAttenuationLinear = ls.mAttenuationLinear;
			gl.mAttenuationQuadratic = ls.mAttenuationQuadratic;
			gl.mColorAmbient = glm::vec4(ls.mColorAmbient, 0.0f);
			gl.mColorDiffuse = glm::vec4(ls.mColorDiffuse, 0.0f);
			gl.mColorSpecular = glm::vec4(ls.mColorSpecular, 0.0f);
			gl.mDirection = glm::vec4(ls.mDirection, 0.0f);
			gl.mPosition = glm::vec4(ls.mPosition, 0.0f);
			gl.mType = ls.mType;
		}
		return gpuLights;
	}

}