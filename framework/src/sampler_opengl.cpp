#include <cg_base.hpp>

namespace cgb
{
	owning_resource<sampler_t> sampler_t::create(filter_mode pFilterMode, border_handling_mode pBorderHandlingMode, context_specific_function<void(sampler_t&)> pAlterConfigBeforeCreation)
	{
		sampler_t result;
		result.mTracker.setTrackee(result);
		return result;
	}
}
