#include <cg_base.hpp>

namespace cgb
{
	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<attachment> pAttachments, std::vector<image_view> _ImageViews, uint32_t _Width, uint32_t _Height, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		framebuffer_t result;
		return result;
	}

	owning_resource<framebuffer_t> framebuffer_t::create(std::vector<image_view> _ImageViews, uint32_t _Width, uint32_t _Height, cgb::context_specific_function<void(framebuffer_t&)> _AlterConfigBeforeCreation)
	{
		framebuffer_t result;
		return result;
	}

}
