#include <cg_base.hpp>

namespace cgb
{
	
	owning_resource<image_view_t> image_view_t::create(cgb::image aImageToOwn, std::optional<image_format> _ViewFormat, context_specific_function<void(image_view_t&)> aAlterConfigBeforeCreation)
	{
		image_view_t result;
		return result;
	}

	attachment attachment::create_for(const image_view_t& _ImageView, std::optional<uint32_t> pLocation)
	{
		throw cgb::runtime_error("not implemented");
	}
}
