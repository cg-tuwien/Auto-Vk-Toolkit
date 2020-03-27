#pragma once

namespace cgb
{
	class standard_descriptor_cache : public descriptor_cache_interface
	{
	public:
		const descriptor_set_layout& get_or_alloc_layout(descriptor_set_layout& aPreparedLayout) override;

	private:
		std::unordered_set<descriptor_set_layout> mLayouts;
	};
}
