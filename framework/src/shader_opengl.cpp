#include <cg_base.hpp>

namespace cgb
{
	shader shader::prepare(shader_info pInfo)
	{
		shader result;
		result.mInfo = std::move(pInfo);
		result.mTracker.setTrackee(result);
		return result;
	}

	shader shader::create(shader_info pInfo)
	{
		auto shdr = shader::prepare(std::move(pInfo));
		return shdr;
	}


}
