#include <cg_base.hpp>

namespace cgb
{
	owning_resource<image_sampler_t> image_sampler_t::create(image_view pImageView, sampler pSampler)
	{
		image_sampler_t result;
		//result.mImageView = std::move(cgb::get(pImageView));
		//result.mSampler = std::move(cgb::get(pSampler));
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal) // TODO: Get this right!
			.setImageView(result.view_handle())
			.setSampler(result.sampler_handle());
		result.mDescriptorType = vk::DescriptorType::eCombinedImageSampler;
		return result;
	}
}
