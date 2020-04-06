#pragma once

namespace cgb
{
	/** Represents a Vulkan framebuffer, holds the native handle and takes
	*	care about their lifetime management.
	*/
	class framebuffer_t
	{
	public:
		framebuffer_t() = default;
		framebuffer_t(framebuffer_t&&) noexcept = default;
		framebuffer_t(const framebuffer_t&) = delete;
		framebuffer_t& operator=(framebuffer_t&&) noexcept = default;
		framebuffer_t& operator=(const framebuffer_t&) = delete;
		~framebuffer_t() = default;

		static owning_resource<framebuffer_t> create(renderpass aRenderpass, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static owning_resource<framebuffer_t> create(std::vector<attachment> pAttachments, std::vector<image_view> aImageViews, uint32_t aWidth, uint32_t aHeight, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static owning_resource<framebuffer_t> create(renderpass aRenderpass, std::vector<image_view> aImageViews, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});
		static owning_resource<framebuffer_t> create(std::vector<attachment> aAttachments, std::vector<image_view> aImageViews, cgb::context_specific_function<void(framebuffer_t&)> aAlterConfigBeforeCreation = {});

		const auto& get_renderpass() const { return mRenderpass; }
		const auto& image_views() const { return mImageViews; }
		const auto& image_view_at(size_t i) const { return mImageViews[i]; }
		auto& get_renderpass() { return mRenderpass; }
		auto& image_views() { return mImageViews; }
		auto& image_view_at(size_t i) { return mImageViews[i]; }
		const auto& create_info() const { return mCreateInfo; }
		const auto& handle() const { return mFramebuffer.get(); }

	private:
		renderpass mRenderpass;
		std::vector<image_view> mImageViews;
		vk::FramebufferCreateInfo mCreateInfo;
		vk::UniqueFramebuffer mFramebuffer;
		context_tracker<framebuffer_t> mTracker;
	};

	using framebuffer = owning_resource<framebuffer_t>;

}
