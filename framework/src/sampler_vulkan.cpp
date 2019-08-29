#include <cg_base.hpp>

namespace cgb
{
	owning_resource<sampler_t> sampler_t::create(filter_mode pFilterMode, border_handling_mode pBorderHandlingMode, context_specific_function<void(sampler_t&)> pAlterConfigBeforeCreation)
	{
		vk::Filter magFilter;
		vk::Filter minFilter;
		vk::SamplerMipmapMode mipmapMode;
		vk::Bool32 enableAnisotropy = VK_FALSE;
		float maxAnisotropy = 1.0f;
		switch (pFilterMode)
		{
		case cgb::filter_mode::nearest_neighbor:
			magFilter = vk::Filter::eNearest;
			minFilter = vk::Filter::eNearest;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case cgb::filter_mode::bilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eNearest;
			break;
		case cgb::filter_mode::trilinear:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear; // TODO: Create MIP-maps!
			break;
		case cgb::filter_mode::cubic: // I have no idea what I'm doing.
			magFilter = vk::Filter::eCubicIMG;
			minFilter = vk::Filter::eCubicIMG;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			break;
		case cgb::filter_mode::anisotropic_2x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 2.0f;
			break;
		case cgb::filter_mode::anisotropic_4x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 4.0f;
			break;
		case cgb::filter_mode::anisotropic_8x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 8.0f;
			break;
		case cgb::filter_mode::anisotropic_16x:
			magFilter = vk::Filter::eLinear;
			minFilter = vk::Filter::eLinear;
			mipmapMode = vk::SamplerMipmapMode::eLinear;
			enableAnisotropy = VK_TRUE;
			maxAnisotropy = 16.0f;
			break;
		default:
			throw std::runtime_error("invalid cgb::filter_mode");
		}

		// Determine how to handle the borders:
		vk::SamplerAddressMode addressMode;
		switch (pBorderHandlingMode)
		{
		case cgb::border_handling_mode::clamp_to_edge:
			addressMode = vk::SamplerAddressMode::eClampToEdge;
			break;
		case cgb::border_handling_mode::mirror_clamp_to_edge:
			addressMode = vk::SamplerAddressMode::eMirrorClampToEdge;
			break;
		case cgb::border_handling_mode::clamp_to_border:
			addressMode = vk::SamplerAddressMode::eClampToEdge;
			break;
		case cgb::border_handling_mode::repeat:
			addressMode = vk::SamplerAddressMode::eRepeat;
			break;
		case cgb::border_handling_mode::mirrored_repeat:
			addressMode = vk::SamplerAddressMode::eMirroredRepeat;
			break;
		default:
			throw std::runtime_error("invalid cgb::border_handling_mode");
		}

		// Compile the config for this sampler:
		sampler_t result;
		result.mInfo = vk::SamplerCreateInfo()
			.setMagFilter(magFilter)
			.setMinFilter(minFilter)
			.setAddressModeU(addressMode)
			.setAddressModeV(addressMode)
			.setAddressModeW(addressMode)
			.setAnisotropyEnable(enableAnisotropy)
			.setMaxAnisotropy(maxAnisotropy)
			.setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
			// The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image. 
			// If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range.
			// If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes. Real-world applications almost 
			// always use normalized coordinates, because then it's possible to use textures of varying resolutions with the exact 
			// same coordinates. [4]
			.setUnnormalizedCoordinates(VK_FALSE)
			// If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison 
			// is used in filtering operations. This is mainly used for percentage-closer filtering on shadow maps. [4]
			.setCompareEnable(VK_FALSE)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(mipmapMode)
			.setMipLodBias(0.0f)
			.setMinLod(0.0f)
			.setMaxLod(0.0f);

		// Call custom config function
		if (pAlterConfigBeforeCreation.mFunction) {
			pAlterConfigBeforeCreation.mFunction(result);
		}

		result.mSampler = context().logical_device().createSamplerUnique(result.config());
		result.mDescriptorInfo = vk::DescriptorImageInfo{}
			.setSampler(result.handle());
		result.mDescriptorType = vk::DescriptorType::eSampler;
		result.mTracker.setTrackee(result);
		return result;
	}
}
