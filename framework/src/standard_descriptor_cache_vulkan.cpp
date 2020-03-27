#include <cg_base.hpp>

namespace cgb
{
	const descriptor_set_layout& standard_descriptor_cache::get_or_alloc_layout(descriptor_set_layout& aPreparedLayout)
	{
		const auto it = mLayouts.find(aPreparedLayout);
		if (mLayouts.end() != it) {
			assert(it->handle());
			return *it;
		}
		aPreparedLayout.allocate();
		const auto result = mLayouts.insert(std::move(aPreparedLayout));
		assert(result.second);
		return *result.first;
	}
}
