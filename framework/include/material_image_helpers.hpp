#pragma once

namespace cgb
{
	
	static image create_image_from_file(const std::string& _Path)
	{
		int width, height, channels;
		stbi_set_flip_vertically_on_load(true);
		stbi_uc* pixels = stbi_load(_Path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
		size_t imageSize = width * height * 4;

		if (!pixels) {
			throw std::runtime_error("Couldn't load image using stbi_load");
		}

		auto stagingBuffer = cgb::create_and_fill(
			cgb::generic_buffer_meta::create_from_size(imageSize),
			cgb::memory_usage::host_coherent,
			pixels,
			nullptr,
			vk::BufferUsageFlagBits::eTransferSrc);
			
		stbi_image_free(pixels);

		auto img = cgb::image_t::create(width, height, cgb::image_format(vk::Format::eR8G8B8A8Unorm), cgb::memory_usage::device);
		cgb::transition_image_layout(img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		cgb::copy_buffer_to_image(stagingBuffer, img);
		cgb::transition_image_layout(img, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		// TODO: Clearly, this has to stop :O
		cgb::context().logical_device().waitIdle();

		return img;
	}


	extern std::tuple<std::vector<material_gpu_data>, std::vector<image_sampler>> convert_for_gpu_usage(std::vector<cgb::material_config> _MaterialConfigs);
}
