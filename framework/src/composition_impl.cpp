#include "composition_impl.h"

namespace cgb
{
	cgb::time& composition_impl::time() override
	{
		
	}

	cg_object* composition_impl::object_at_index(int32_t) override
	{
		
	}
	cg_object* composition_impl::object_by_name(std::string) override;
	void composition_impl::start() override;
	void composition_impl::stop() override;
}
