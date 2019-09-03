#include <cg_base.hpp>

namespace cgb
{
	owning_resource<image_sampler_t> image_sampler_t::create(image_view pImageView, sampler pSampler)
	{
		image_sampler_t result;
		result.mImageView = std::move(pImageView);
		result.mSampler = std::move(pSampler);

		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setImageView(result.view_handle())
			.setSampler(result.sampler_handle());
		if (pImageView->has_image_t()) {
			result.mDescriptorInfo.setImageLayout(pImageView->get_image().target_layout());
		}
		else {
			result.mDescriptorInfo.setImageLayout(vk::ImageLayout::eGeneral); 
		}
		
		result.mDescriptorType = vk::DescriptorType::eCombinedImageSampler;
		return result;
	}
}
