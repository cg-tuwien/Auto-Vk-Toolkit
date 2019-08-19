#include <cg_base.hpp>

namespace cgb
{
	cgb::owning_resource<semaphore_t> semaphore_t::create(context_specific_function<void(semaphore_t&)> _AlterConfigBeforeCreation)
	{ 
		semaphore_t result;
		return result;
	}
}
