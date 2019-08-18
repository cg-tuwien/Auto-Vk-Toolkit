#pragma once

namespace cgb
{
	/** Represents one specific native image format for the Vulkan context */
	struct image_format
	{
		image_format() noexcept;
		image_format(const vk::Format& pFormat) noexcept;
		image_format(const vk::SurfaceFormatKHR& pSrfFrmt) noexcept;

		vk::Format mFormat;
	};

	/** Represents an image and its associated memory
	 */
	class image_t
	{
	public:
		image_t() = default;
		image_t(const image_t&) = delete;
		image_t(image_t&&) = default;
		image_t& operator=(const image_t&) = delete;
		image_t& operator=(image_t&&) = default;
		~image_t() = default;

		/** Get the config which is used to created this image with the API. */
		const auto& config() const { return mInfo; }
		/** Get the config which is used to created this image with the API. */
		auto& config() { return mInfo; }
		/** Gets the image handle. */
		const auto& image_handle() const { return mImage.get(); }
		/** Gets the handle to the image's memory. */
		const auto& memory_handle() const { return mMemory.get(); }

		/** Creates a new image
		 *	@param	pWidth						The width of the image to be created
		 *	@param	pHeight						The height of the image to be created
		 *	@param	pFormat						The image format of the image to be created
		 *	@param	pMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		 *	@param	pUseMipMaps					Whether or not MIP maps shall be created for this image. Specifying `true` will set the maximum number of MIP map images.
		 *	@param	pNumLayers					How many layers the image to be created shall contain.
		 *	@param	pAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		 *	@return	Returns a newly created image.
		 */
		static owning_resource<image_t> create(int pWidth, int pHeight, image_format pFormat, memory_usage pMemoryUsage, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

		/** Creates a new image
		*	@param	pWidth						The width of the depth buffer to be created
		*	@param	pHeight						The height of the depth buffer to be created
		*	@param	pFormat						The image format of the image to be created, or a default depth format if not specified.
		*	@param	pMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		*	@param	pUseMipMaps					Whether or not MIP maps shall be created for this image. Specifying `true` will set the maximum number of MIP map images.
		*	@param	pNumLayers					How many layers the image to be created shall contain.
		*	@param	pAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created depth buffer.
		*/
		static owning_resource<image_t> create_depth(int pWidth, int pHeight, std::optional<image_format> pFormat = std::nullopt, memory_usage pMemoryUsage = memory_usage::device, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

		/** Creates a new image
		*	@param	pWidth						The width of the depth+stencil buffer to be created
		*	@param	pHeight						The height of the depth+stencil buffer to be created
		*	@param	pFormat						The image format of the image to be created, or a default depth format if not specified.
		*	@param	pMemoryUsage				Where the memory of the image shall be allocated (GPU or CPU) and how it is going to be used.
		*	@param	pUseMipMaps					Whether or not MIP maps shall be created for this image. Specifying `true` will set the maximum number of MIP map images.
		*	@param	pNumLayers					How many layers the image to be created shall contain.
		*	@param	pAlterConfigBeforeCreation	A context-specific function which allows to modify the `vk::ImageCreateInfo` just before the image will be created. Use `.config()` to access the configuration structure!
		*	@return	Returns a newly created depth+stencil buffer.
		*/
		static owning_resource<image_t> create_depth_stencil(int pWidth, int pHeight, std::optional<image_format> pFormat = std::nullopt, memory_usage pMemoryUsage = memory_usage::device, bool pUseMipMaps = false, int pNumLayers = 1, context_specific_function<void(image_t&)> pAlterConfigBeforeCreation = {});

		// TODO: What to do with this one: ??
		vk::ImageMemoryBarrier create_barrier(vk::AccessFlags pSrcAccessMask, vk::AccessFlags pDstAccessMask, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, std::optional<vk::ImageSubresourceRange> pSubresourceRange = std::nullopt) const;

	private:
		// The image create info which contains all the parameters for image creation
		vk::ImageCreateInfo mInfo;
		// The image handle. This member will contain a valid handle only after successful image creation.
		vk::UniqueImage mImage;
		// The memory handle. This member will contain a valid handle only after successful image creation.
		vk::UniqueDeviceMemory mMemory;
	};

	/** Typedef representing any kind of OWNING image representations. */
	using image	= owning_resource<image_t>;






	// ============================= TODO/WIP ============================

	extern vk::ImageMemoryBarrier create_image_barrier(vk::Image pImage, vk::Format pFormat, vk::AccessFlags pSrcAccessMask, vk::AccessFlags pDstAccessMask, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout, std::optional<vk::ImageSubresourceRange> pSubresourceRange = std::nullopt);

	extern void transition_image_layout(const image_t& pImage, vk::Format pFormat, vk::ImageLayout pOldLayout, vk::ImageLayout pNewLayout);

	template <typename Bfr>
	void copy_buffer_to_image(const Bfr& pSrcBuffer, const image_t& pDstImage)
	{
		//auto commandBuffer = context().create_command_buffers_for_transfer(1);
		auto commandBuffer = cgb::context().transfer_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		// Immediately start recording the command buffer:
		commandBuffer.begin_recording();

		auto copyRegion = vk::BufferImageCopy()
			.setBufferOffset(0)
			// The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. For example, you could have some padding 
			// bytes between rows of the image. Specifying 0 for both indicates that the pixels are simply tightly packed like they are in our case. [3]
			.setBufferRowLength(0)
			.setBufferImageHeight(0)
			.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0u)
				.setBaseArrayLayer(0u)
				.setLayerCount(1u))
			.setImageOffset({ 0u, 0u, 0u })
			.setImageExtent(pDstImage.mInfo.extent);

		commandBuffer.handle().copyBufferToImage(
			pSrcBuffer.buffer_handle(), 
			pDstImage.mImage, 
			vk::ImageLayout::eTransferDstOptimal,
			{ copyRegion });

		// That's all
		commandBuffer.end_recording();

		auto submitInfo = vk::SubmitInfo()
			.setCommandBufferCount(1u)
			.setPCommandBuffers(commandBuffer.handle_addr());
		cgb::context().transfer_queue().handle().submit({ submitInfo }, nullptr); // not using fence... TODO: maybe use fence!
		cgb::context().transfer_queue().handle().waitIdle();
	}

}
