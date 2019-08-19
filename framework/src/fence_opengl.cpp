#include <cg_base.hpp>

namespace cgb
{
	cgb::owning_resource<fence_t> fence_t::create(bool _CreateInSignaledState, context_specific_function<void(fence_t&)> _AlterConfigBeforeCreation)
	{
		fence_t result;
		return result;
	}
}
