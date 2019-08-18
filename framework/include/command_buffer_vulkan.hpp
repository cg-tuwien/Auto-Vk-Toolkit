#pragma once

namespace cgb // ========================== TODO/WIP =================================
{
	class command_pool;

	/** A command buffer which has been created for a certain queue family */
	class command_buffer
	{
	public:
		command_buffer() = default;
		command_buffer(command_buffer&&) = default;
		command_buffer(const command_buffer&) = delete;
		command_buffer& operator=(command_buffer&&) = default;
		command_buffer& operator=(const command_buffer&) = delete;
		~command_buffer() = default;

		void begin_recording();
		void end_recording();
		void begin_render_pass(const vk::RenderPass& pRenderPass, const vk::Framebuffer& pFramebuffer, const vk::Offset2D& pOffset, const vk::Extent2D& pExtent);
		void set_image_barrier(const vk::ImageMemoryBarrier& pBarrierInfo);
		void copy_image(const image_t& pSource, const vk::Image& pDestination);
		void end_render_pass();

		auto& begin_info() const { return mBeginInfo; }
		auto& handle() const { return mCommandBuffer.get(); }
		auto* handle_addr() const { return &mCommandBuffer.get(); }

		static std::vector<command_buffer> create_many(uint32_t pCount, command_pool& pPool, vk::CommandBufferUsageFlags pUsageFlags);
		static command_buffer create(command_pool& pPool, vk::CommandBufferUsageFlags pUsageFlags);

	private:
		vk::CommandBufferBeginInfo mBeginInfo;
		vk::UniqueCommandBuffer mCommandBuffer;
	};

}
