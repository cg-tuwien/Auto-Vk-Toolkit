#include <cg_base.hpp>

namespace cgb
{
	attachment attachment::create_color(image_format pFormat, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is a depth format, but is used for a color attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			1,				// num samples
			false			// => no need to resolve
		};
	}

	attachment attachment::create_depth(std::optional<image_format> pFormat, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
		if (!pFormat.has_value()) {
			pFormat = image_format::default_depth_format();
		}

#if defined(_DEBUG)
		if (!is_depth_format(pFormat.value())) {
			LOG_WARNING("The specified image_format is probably not a depth format, but is used for a depth attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat.value(),
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			1,				// num samples
			false			// => no need to resolve
		};
	}

	attachment attachment::create_depth_stencil(std::optional<image_format> pFormat, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
		if (!pFormat.has_value()) {
			pFormat = image_format::default_depth_stencil_format();
		}
		return create_depth(std::move(pFormat), std::move(pImageUsage), std::move(pLocation));
	}

	attachment attachment::create_shader_input(image_format pFormat, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
		return attachment{
			pLocation,
			pFormat,
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			1,				// num samples
			false			// => no need to resolve
		};
	}

	attachment attachment::create_color_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (!is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is a depth format, but is used for a color attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			pSampleCount,				// num samples
			pResolveMultisamples		// do it or don't?
		};
	}

	attachment attachment::create_depth_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
#if defined(_DEBUG)
		if (!is_depth_format(pFormat)) {
			LOG_WARNING("The specified image_format is probably not a depth format, but is used for a depth attachment.");
		}
#endif
		return attachment{
			pLocation,
			pFormat,
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			pSampleCount,				// num samples
			pResolveMultisamples		// do it or don't?
		};
	}

	attachment attachment::create_shader_input_multisampled(image_format pFormat, int pSampleCount, bool pResolveMultisamples, image_usage pImageUsage, std::optional<uint32_t> pLocation)
	{
		return attachment{
			pLocation,
			pFormat,
			pImageUsage,
			cfg::attachment_load_operation::dont_care,
			cfg::attachment_store_operation::dont_care,
			{}, {},
			pSampleCount,				// num samples
			pResolveMultisamples		// do it or don't?
		};
	}

	attachment& attachment::set_load_operation(cfg::attachment_load_operation aLoadOp)
	{
		mLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_store_operation(cfg::attachment_store_operation aStoreOp)
	{
		mStoreOperation = aStoreOp;
		return *this;
	}

	attachment& attachment::load_contents()
	{
		return set_load_operation(cfg::attachment_load_operation::load);
	}
	
	attachment& attachment::clear_contents()
	{
		return set_load_operation(cfg::attachment_load_operation::clear);
	}
	
	attachment& attachment::store_contents()
	{
		return set_store_operation(cfg::attachment_store_operation::store);
	}

	attachment& attachment::set_stencil_load_operation(cfg::attachment_load_operation aLoadOp)
	{
		mStencilLoadOperation = aLoadOp;
		return *this;
	}
	
	attachment& attachment::set_stencil_store_operation(cfg::attachment_store_operation aStoreOp)
	{
		mStencilStoreOperation = aStoreOp;
		return *this;
	}
	
	attachment& attachment::load_stencil_contents()
	{
		return set_stencil_load_operation(cfg::attachment_load_operation::load);
	}
	
	attachment& attachment::clear_stencil_contents()
	{
		return set_stencil_load_operation(cfg::attachment_load_operation::clear);
	}
	
	attachment& attachment::store_stencil_contents()
	{
		return set_stencil_store_operation(cfg::attachment_store_operation::store);
	}
}