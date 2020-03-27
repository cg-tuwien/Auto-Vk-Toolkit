#pragma once

namespace cgb
{
	class descriptor_set_layout;
	
	class descriptor_cache_interface
	{
	public:
		virtual ~descriptor_cache_interface() = default;
	
		virtual const descriptor_set_layout& get_or_alloc_layout(descriptor_set_layout& aPreparedLayout) = 0;
	};
}
