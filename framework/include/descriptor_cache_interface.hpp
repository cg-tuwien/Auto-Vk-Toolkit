#pragma once

namespace cgb
{
	class descriptor_set_layout;
	class descriptor_set;
	
	class descriptor_cache_interface
	{
	public:
		virtual ~descriptor_cache_interface() = default;
	
		virtual const descriptor_set_layout& get_or_alloc_layout(descriptor_set_layout aPreparedLayout) = 0;

		virtual const descriptor_set* get_descriptor_set_from_cache(const descriptor_set& aPreparedSet) = 0;

		virtual std::vector<const descriptor_set*> alloc_descriptor_sets(const std::vector<std::reference_wrapper<const descriptor_set_layout>>& aLayouts, std::vector<descriptor_set> aPreparedSets) = 0;

		virtual void clear_all_descriptor_sets() = 0;

		virtual void clear_all_layouts() = 0;
	};
}
