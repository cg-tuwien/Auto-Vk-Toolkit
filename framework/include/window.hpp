#pragma once

namespace xk
{
	class window : public window_base
	{
		friend class generic_glfw;
		friend class vulkan;
	public:
		// A mutex used to protect concurrent command buffer submission
		static std::mutex sSubmitMutex;
		
		window() = default;
		window(window&&) noexcept = default;
		window(const window&) = delete;
		window& operator =(window&&) noexcept = default;
		window& operator =(const window&) = delete;
		~window() = default;

		/** Set to true to create a resizable window, set to false to prevent the window from being resized */
		void enable_resizing(bool aEnable);
		
		/** Request a framebuffer for this window which is capable of sRGB formats */
		void request_srgb_framebuffer(bool aRequestSrgb);

		/** Sets the presentation mode for this window's swap chain. */
		void set_presentaton_mode(xk::presentation_mode aMode);

		/** Sets the number of samples for MSAA */
		void set_number_of_samples(int aNumSamples);

		/** Sets the number of presentable images for a swap chain */
		void set_number_of_presentable_images(uint32_t aNumImages);

		/** Sets the number of images which can be rendered into concurrently,
		 *	i.e. the number of "frames in flight"
		 */
		void set_number_of_concurrent_frames(uint32_t aNumConcurrent);

		/** Sets additional attachments which shall be added to the back buffer 
		 *	in addition to the obligatory color attachment.  
		 */
		void set_additional_back_buffer_attachments(std::vector<ak::attachment> aAdditionalAttachments);

		/** Creates or opens the window */
		void open();

		/** Returns true if the window shall be resizable, false if it shall not be. */
		bool get_config_shall_be_resizable() const;
		
		/** Gets the requested surface format for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::SurfaceFormatKHR get_config_surface_format(const vk::SurfaceKHR& aSurface);

		/** Gets the requested presentation mode for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::PresentModeKHR get_config_presentation_mode(const vk::SurfaceKHR& aSurface);

		/**	Gets the number of samples that has been configured.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::SampleCountFlagBits get_config_number_of_samples();

		/** Gets the multisampling-related config info struct for the Vk-pipeline config.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::PipelineMultisampleStateCreateInfo get_config_multisample_state_create_info();

		/** Get the minimum number of concurrent/presentable images for a swap chain.
		*	If no value is set, the surfaces minimum number + 1 will be returned.
		*/
		uint32_t get_config_number_of_presentable_images();

		/** Get the number of concurrent frames.
		*	If no value is explicitely set, the same number as the number of presentable images will be returned.
		*/
		uint32_t get_config_number_of_concurrent_frames();

		/**	Gets the descriptions of the additional back buffer attachments
		 */
		std::vector<ak::attachment> get_additional_back_buffer_attachments();

		/** Gets this window's surface */
		const auto& surface() const { 
			return mSurface.get(); 
		}
		/** Gets this window's swap chain */
		const auto& swap_chain() const { 
			return mSwapChain.get(); 
		}
		/** Gets this window's swap chain's image format */
		const auto& swap_chain_image_format() const { 
			return mSwapChainImageFormat; 
		}
		/** Gets this window's swap chain's color space */
		const auto& swap_chain_color_space() const { 
			return mSwapChainColorSpace; 
		}
		/** Gets this window's swap chain's dimensions */
		auto swap_chain_extent() const {
			return mSwapChainExtent; 
		}
		/** Gets this window's swap chain's image at the specified index. */
		auto& swap_chain_image_at_index(size_t aIdx) { 
			return mSwapChainImageViews[aIdx]->get_image(); 
		}
		/** Gets a collection containing all this window's swap chain image views. */
		const auto& swap_chain_image_views() { 
			return mSwapChainImageViews; 
		}
		/** Gets this window's swap chain's image view at the specified index. */
		const auto& swap_chain_image_view_at_index(size_t aIdx) { 
			return mSwapChainImageViews[aIdx]; 
		}

		/** Gets a collection containing all this window's back buffers. */
		const auto& backbuffers() { 
			return mBackBuffers; 
		}
		/** Gets this window's back buffer at the specified index. */
		const auto& backbuffer_at_index(size_t aIdx) { 
			return mBackBuffers[aIdx]; 
		}

		/** Gets the number of how many frames are (potentially) concurrently rendered into,
		 *	or put differently: How many frames are (potentially) "in flight" at the same time.
		 */
		auto number_of_in_flight_frames() const { 
			return static_cast<int64_t>(mFences.size());
		}

		/** Gets the number of images there are in the swap chain */
		auto number_of_swapchain_images() const {
			return static_cast<int64_t>(mSwapChainImageViews.size());
		}
		
		/** Gets the current frame index. */
		auto current_frame() const { 
			return mCurrentFrame; 
		}

		/** Returns the "in flight index" for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		auto in_flight_index_for_frame(std::optional<int64_t> aFrameId = {}) const { 
			return aFrameId.value_or(current_frame()) % number_of_in_flight_frames(); 
		}

		/** Returns the "swapchain image index" for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		auto swapchain_image_index_for_frame(std::optional<int64_t> aFrameId = {}) const {
			return aFrameId.value_or(current_frame()) % number_of_swapchain_images();
		}

		/** Returns the backbuffer for the requested frame 
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		auto& backbufer_for_frame(std::optional<int64_t> aFrameId = {}) {
			return mBackBuffers[swapchain_image_index_for_frame(aFrameId)];
		}
		
		/** Returns the swap chain image for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		auto& image_for_frame(std::optional<int64_t> aFrameId = {}) {
			return swap_chain_image_at_index(swapchain_image_index_for_frame(aFrameId));
		}

		/** Returns the swap chain image view for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		ak::image_view_t& image_view_for_frame(std::optional<int64_t> aFrameId = {}) {
			return mSwapChainImageViews[swapchain_image_index_for_frame(aFrameId)];
		}

		/** Returns the fence for the requested frame, which depends on the frame's "in flight index.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		ak::fence_t& fence_for_frame(std::optional<int64_t> aFrameId = {}) {
			return mFences[in_flight_index_for_frame(aFrameId)];
		}

		/** Returns the "image available"-semaphore for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		ak::semaphore_t& image_available_semaphore_for_frame(std::optional<int64_t> aFrameId = {}) {
			return mImageAvailableSemaphores[swapchain_image_index_for_frame(aFrameId)];
		}

		/** Returns the "render finished"-semaphore for the requested frame, which depends on the frame's "in flight index.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		auto& render_finished_semaphore_for_frame(std::optional<int64_t> aFrameId = {}) {
			return mRenderFinishedSemaphores[in_flight_index_for_frame(aFrameId)];
		}

		/**	Add an extra semaphore to wait on for the given frame id.
		 *	@param	aSemaphore		The semaphore to take ownership for and to set as dependency for a (future) frame.
		 *	@param	aFrameId		The (future) frame-id which this semaphore shall be a dependency for.
		 *							If the parameter is not set, the semaphore will be assigned to the current_frame()-id,
		 *							which means for the next frame which will be rendered. The next frame which will be 
		 *							rendered is the frame with the id current_frame(), assuming this function is called 
		 *							before render_frame() is called.
		 */
		void set_extra_semaphore_dependency(ak::semaphore aSemaphore, std::optional<int64_t> aFrameId = {});

		/**	Pass a "single use" command buffer for the given frame and have its lifetime handled.
		 *	The lifetime of this command buffer will last until the given frame + number of frames in flight.
		 *	@param	aCommandBuffer	The command buffer to take ownership of and to handle lifetime of.
		 *	@param	aFrameId		The frame this command buffer is associated to.
		 */
		void handle_single_use_command_buffer_lifetime(ak::command_buffer aCommandBuffer, std::optional<int64_t> aFrameId = {});

		/**	Pass a "single use" command buffer for the given frame and have its lifetime handled.
		 *	The submitted command buffer's commands have an execution dependency on the back buffer's
		 *	image to become available.
		 *	Put differently: No commands will execute until the referenced frame's swapchain image has become available.
		 *	@param	aCommandBuffer	The command buffer to take ownership of and to handle lifetime of.
		 *	@param	aFrameId		The frame this command buffer is associated to.
		 */
		void submit_for_backbuffer(ak::command_buffer aCommandBuffer, std::optional<int64_t> aFrameId = {});

		/**	Pass a "single use" command buffer for the given frame and have its lifetime handled.
		 *	The submitted command buffer's commands have an execution dependency on the back buffer's
		 *	image to become available.
		 *	Put differently: No commands will execute until the referenced frame's swapchain image has become available.
		 *	@param	aCommandBuffer	The command buffer to take ownership of and to handle lifetime of.
		 *	@param	aFrameId		The frame this command buffer is associated to.
		 */
		void submit_for_backbuffer(std::optional<ak::command_buffer> aCommandBuffer, std::optional<int64_t> aFrameId = {});

		/**	Pass a reference to a command buffer and submit it after the given frame's back buffer has become available.
		 *	The submitted command buffer's commands have an execution dependency on the back buffer's
		 *	image to become available (same characteristics as `submit_for_backbuffer` in that matter).
		 *	Attention: The application must ensure that the command buffer lives long enough!
		 *			   I.e., it is the application's responsibility to properly handle the command buffer's lifetime.
		 *	@param	aCommandBuffer	The command buffer to take ownership of and to handle lifetime of.
		 *	@param	aFrameId		The frame this command buffer is associated to.
		 */
		void submit_for_backbuffer_ref(const ak::command_buffer_t& aCommandBuffer, std::optional<int64_t> aFrameId = {});

		/**	Remove all the semaphores which were dependencies for one of the previous frames, but
		 *	can now be safely destroyed.
		 */
		std::vector<ak::semaphore> remove_all_extra_semaphore_dependencies_for_frame(int64_t aPresentFrameId);

		/** Remove all the "single use" command buffers for the given frame, also clear command buffer references.
		 *	The command buffers are moved out of the internal storage and returned to the caller.
		 */
		std::vector<ak::command_buffer> clean_up_command_buffers_for_frame(int64_t aPresentFrameId);

		std::vector<std::reference_wrapper<const ak::command_buffer_t>> get_command_buffer_refs_for_frame(int64_t aFrameId) const;
		
		void fill_in_extra_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId) const;

		void fill_in_extra_render_finished_semaphores_for_frame(std::vector<vk::Semaphore>& aSemaphores, int64_t aFrameId) const;

		//std::vector<semaphore> set_num_extra_semaphores_to_generate_per_frame(uint32_t _NumExtraSemaphores);

		/**
		 *	Called BEFORE all the render callbacks are invoked.
		 *	This is where fences are waited on and resources are freed.
		 */
		void sync_before_render();
		
		/**
		 *	This is THE "render into backbuffer" method.
		 *	Invoked every frame internally, from `composition.hpp`
		 */
		void render_frame();

		/**	Gets the handle of the renderpass.
		 */
		const auto& renderpass_handle() const { return (*mBackBufferRenderpass).handle(); }

		/** Gets a const reference to the backbuffer's render pass
		 */
		const ak::renderpass_t& get_renderpass() const { return mBackBufferRenderpass; }

		/**	A convenience method that internally calls `cgb::copy_image_to_another` and establishes rather coarse barriers
		 *	for synchronization by using some predefined synchronization functions from `cgb::sync::presets::image_copy`.
		 *
		 *	For tighter synchronization, feel free to copy&modify&paste the code of this method.
		 *
		 *	Source:
		 *
		 *		auto& destinationImage = image_for_frame(aDestinationFrameId);
		 *		return copy_image_to_another(aSourceImage, destinationImage, cgb::sync::with_barriers_by_return(
		 *				cgb::sync::presets::image_copy::wait_for_previous_operations(aSourceImage, destinationImage),
		 *				aShallBePresentedDirectlyAfterwards 
		 *					? cgb::sync::presets::image_copy::directly_into_present(aSourceImage, destinationImage)
		 *					: cgb::sync::presets::image_copy::restore_layout_and_let_subsequent_operations_wait(aSourceImage, destinationImage)
		 *			),
		 *			true, // Restore layout of source image
		 *			false // Don't restore layout of destination => this is handled by the after-handler in any case
		 *		);
		 *		
		 */
		std::optional<ak::command_buffer> copy_to_swapchain_image(ak::image_t& aSourceImage, std::optional<int64_t> aDestinationFrameId, bool aShallBePresentedDirectlyAfterwards);

		/**	A convenience method that internally calls `cgb::copy_image_to_another` and establishes rather coarse barriers
		 *	for synchronization by using some predefined synchronization functions from `cgb::sync::presets::image_copy`.
		 *
		 *	For tighter synchronization, feel free to copy&modify&paste the code of this method.
		 *
		 *	Source:
		 *
		 *		auto& destinationImage = image_for_frame(aDestinationFrameId);
		 *		return blit_image(aSourceImage, image_for_frame(aDestinationFrameId), cgb::sync::with_barriers_by_return(
		 *			cgb::sync::presets::image_copy::wait_for_previous_operations(aSourceImage, destinationImage),
		 *			aShallBePresentedDirectlyAfterwards 
		 *				? cgb::sync::presets::image_copy::directly_into_present(aSourceImage, destinationImage)
		 *				: cgb::sync::presets::image_copy::restore_layout_and_let_subsequent_operations_wait(aSourceImage, destinationImage)
		 *			),	
		 *			true, // Restore layout of source image
		 *			false // Don't restore layout of destination => this is handled by the after-handler in any case
		 *		);
		 *	
		 */
		std::optional<ak::command_buffer> blit_to_swapchain_image(ak::image_t& aSourceImage, std::optional<int64_t> aDestinationFrameId, bool aShallBePresentedDirectlyAfterwards);

		/**	This is intended to be used as a command buffer lifetime handler for `cgb::sync::with_barriers`.
		 *	The specified frame id is the frame where the command buffer has to be guaranteed to finish
		 *	(be it by the means of pipeline barriers, semaphores, or fences) and hence, it will be destroyed
		 *	either number_of_in_flight_frames() or number_of_swapchain_images() frames later.
		 *
		 *	Example usage:
		 *	cgb::sync::with_barriers(cgb::context().main_window().handle_command_buffer_lifetime());
		 */
		auto command_buffer_lifetime_handler(std::optional<int64_t> aFrameId = {})
		{
			return [this, aFrameId](ak::command_buffer aCommandBufferToLifetimeHandle){
				handle_single_use_command_buffer_lifetime(std::move(aCommandBufferToLifetimeHandle), aFrameId);
			};
		}
		
	protected:
		

#pragma region configuration properties
		// A function which returns whether or not the window should be resizable
		bool mShallBeResizable = false;
		
		// A function which returns the surface format for this window's surface
		ak::unique_function<vk::SurfaceFormatKHR(const vk::SurfaceKHR&)> mSurfaceFormatSelector;

		// A function which returns the desired presentation mode for this window's surface
		ak::unique_function<vk::PresentModeKHR(const vk::SurfaceKHR&)> mPresentationModeSelector;

		// A function which returns the MSAA sample count for this window's surface
		ak::unique_function<vk::SampleCountFlagBits()> mNumberOfSamplesGetter;

		// A function which returns the MSAA state for this window's surface
		ak::unique_function<vk::PipelineMultisampleStateCreateInfo()> mMultisampleCreateInfoBuilder;

		// A function which returns the desired number of presentable images in the swap chain
		ak::unique_function<uint32_t()> mNumberOfPresentableImagesGetter;

		// A function which returns the number of images which can be rendered into concurrently
		// According to this number, the number of semaphores and fences will be determined.
		ak::unique_function<uint32_t()> mNumberOfConcurrentFramesGetter;

		// A function which returns attachments which shall be attached to the back buffer
		// in addition to the obligatory color attachment.
		ak::unique_function<std::vector<ak::attachment>()> mAdditionalBackBufferAttachmentsGetter;
#pragma endregion

#pragma region swap chain data for this window surface
		// The frame counter/frame id/frame index/current frame number
		int64_t mCurrentFrame;

		// The window's surface
		vk::UniqueSurfaceKHR mSurface;
		// The swap chain for this surface
		vk::UniqueSwapchainKHR mSwapChain;
		// The swap chain's image format
		vk::Format mSwapChainImageFormat;
		// The swap chain's color space
		vk::ColorSpaceKHR mSwapChainColorSpace;
		// The swap chain's extent
		vk::Extent2D mSwapChainExtent;
		// Queue family indices which have shared ownership of the swap chain images
		std::vector<uint32_t> mQueueFamilyIndices;
		// Image data of the swap chain images
		vk::ImageCreateInfo mImageCreateInfoSwapChain;
		// All the image views of the swap chain
		std::vector<ak::image_view> mSwapChainImageViews; // ...but the image views do!
#pragma endregion

#pragma region indispensable sync elements
		// Fences to synchronize between frames (CPU-GPU synchronization)
		std::vector<ak::fence> mFences; 
		// Semaphores to wait for an image to become available (GPU-GPU synchronization) // TODO: true?
		std::vector<ak::semaphore> mImageAvailableSemaphores; 
		// Semaphores to wait for rendering to finish (GPU-GPU synchronization) // TODO: true?
		std::vector<ak::semaphore> mRenderFinishedSemaphores; 
#pragma endregion

#pragma region extra sync elements, i.e. exta semaphores
		// Extra semaphores for frames.
		// The first element in the tuple refers to the frame id which is affected.
		// The second element in the is the semaphore to wait on.
		// Extra dependency semaphores will be waited on along with the mImageAvailableSemaphores
		std::vector<std::tuple<int64_t, ak::semaphore>> mExtraSemaphoreDependencies;
		 
		// Number of extra semaphores to generate per frame upon fininshing the rendering of a frame
		uint32_t mNumExtraRenderFinishedSemaphoresPerFrame;

		// Contains the extra semaphores to be signalled per frame
		// The length of this vector will be: number_of_concurrent_frames() * mNumExtraSemaphoresPerFrame
		// These semaphores will be signalled together with the mRenderFinishedSemaphores
		std::vector<ak::semaphore> mExtraRenderFinishedSemaphores;
#pragma endregion

		// The renderpass used for the back buffers
		ak::renderpass mBackBufferRenderpass;

		// The backbuffers of this window
		std::vector<ak::framebuffer> mBackBuffers;

		// The render pass for this window's UI calls
		vk::RenderPass mUiRenderPass;

		// This container handles single use-command buffers' lifetimes.
		// It is used for both, such that are committed `cgb::sync::with_barriers_on_current_frame` and
		// for those submitted via `window::submit_for_backbuffer`.
		// A command buffer's lifetime lasts until the specified int64_t frame-id + max(number_of_swapchain_images(), number_of_in_flight_frames())
		std::deque<std::tuple<int64_t, ak::command_buffer>> mSingleUseCommandBuffers;

		// Comand buffers which are submitted via `window::submit_for_backbuffer` or `window::submit_for_backbuffer_ref`
		// are stored within this container. In the former case, they are moved into `mSingleUseCommandBuffers` first,
		// and a reference into `mSingleUseCommandBuffers` ist pushed to the back of `mCommandBufferRefs` afterwards.
		std::vector<std::tuple<int64_t, std::reference_wrapper<const ak::command_buffer_t>>> mCommandBufferRefs;
	};
}
