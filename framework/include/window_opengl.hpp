#pragma once

namespace cgb
{
	class window : public window_base
	{
		friend class generic_glfw;
		friend class opengl;
	public:

		window() = default;
		window(window&&) noexcept = default;
		window(const window&) = delete;
		window& operator =(window&&) noexcept = default;
		window& operator =(const window&) = delete;
		~window() = default;

		/** Request a framebuffer for this window which is capable of sRGB formats */
		void request_srgb_framebuffer(bool pRequestSrgb);

		/** Sets the presentation mode for this window's swap chain. */
		void set_presentaton_mode(cgb::presentation_mode pMode);

		/** Sets the number of samples for MSAA */
		void set_number_of_samples(int pNumSamples);

		/** Sets the number of presentable images for a swap chain */
		void set_number_of_presentable_images(uint32_t pNumImages);

		/** Sets the number of images which can be rendered into concurrently,
		 *	i.e. the number of "frames in flight"
		 */
		void set_number_of_concurrent_frames(uint32_t pNumConcurrent);

		/** Creates or opens the window */
		void open();

		void render_frame(std::vector<std::reference_wrapper<const cgb::command_buffer>> aCommandBufferRefs, std::optional<std::reference_wrapper<const cgb::image_t>> aCopyToPresent = {});

	};
}
