#pragma once

namespace cgb
{
	class window : public window_base
	{
		friend class generic_glfw;
		friend class vulkan;
	public:

		window() = default;
		~window() = default;
		window(const window&) = delete;
		window(window&&) = default;
		window& operator =(const window&) = delete;
		window& operator =(window&&) = default;

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

		/** Gets the requested surface format for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::SurfaceFormatKHR get_config_surface_format(const vk::SurfaceKHR& surface);

		/** Gets the requested presentation mode for the given surface.
		 *	A default value will be set if no other value has been configured.
		 */
		vk::PresentModeKHR get_config_presentation_mode(const vk::SurfaceKHR& surface);

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
		/** Gets this window's swap chain's dimensions */
		auto swap_chain_extent() const {
			return mSwapChainExtent; 
		}
		/** Gets a collection containing all this window's swap chain images. */
		const auto& swap_chain_images() { 
			return mSwapChainImages;
		}
		/** Gets this window's swap chain's image at the specified index. */
		const auto& swap_chain_image_at_index(size_t pIdx) { 
			return mSwapChainImages[pIdx]; 
		}
		/** Gets a collection containing all this window's swap chain image views. */
		const auto& swap_chain_image_views() { 
			return mSwapChainImageViews; 
		}
		/** Gets this window's swap chain's image view at the specified index. */
		const auto& swap_chain_image_view_at_index(size_t pIdx) { 
			return mSwapChainImageViews[pIdx]; 
		}

		/** Gets a collection containing all this window's back buffers. */
		const auto& backbuffers() { 
			return mBackBuffers; 
		}
		/** Gets this window's back buffer at the specified index. */
		const auto& backbuffer_at_index(size_t pIdx) { 
			return mBackBuffers[pIdx]; 
		}

		/** Gets the number of how many images there are in the swap chain. */
		auto number_of_swapchain_images() const {
			return mSwapChainImageViews.size(); 
		}
		/** Gets the number of how many frames are (potentially) concurrently rendered into. */
		auto number_of_concurrent_frames() const { 
			return mFences.size(); 
		}

		/** Gets the current frame index. */
		auto current_frame() const { 
			return mCurrentFrame; 
		}
		/** Helper function which calculates the image index for the given frame index. 
		 *	@param pFrameIndex Frame index which to calculate the corresponding image index for.
		 *	@return Returns the image index for the given frame index.
		 */
		auto calculate_image_index_for_frame(int64_t pFrameIndex) const { 
			return pFrameIndex % number_of_swapchain_images(); 
		}
		/** Helper function which calculates the sync index for the given frame index. 
		*	@param pFrameIndex Frame index which to calculate the corresponding image index for.
		*	@return Returns the sync index for the given frame index.
		*/
		auto calculate_sync_index_for_frame(int64_t pFrameIndex) const {
			return pFrameIndex % number_of_concurrent_frames(); 
		}

		/** Returns the image index for the requested frame.
		 *	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		 *								Specify a negative offset to refer to a previous frame.
		 */
		auto image_index_for_frame(int64_t pCurrentFrameOffset = 0) const { 
			return calculate_image_index_for_frame(static_cast<int64_t>(current_frame()) + pCurrentFrameOffset); 
		}
		/** Returns the sync index for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		auto sync_index_for_frame(int64_t pCurrentFrameOffset = 0) const { 
			return calculate_sync_index_for_frame(static_cast<int64_t>(current_frame()) + pCurrentFrameOffset); 
		}
		
		/** Returns the swap chain image for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& image_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mSwapChainImages[image_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the swap chain image view for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const image_view_t& image_view_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mSwapChainImageViews[image_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the fence for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const fence_t& fence_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mFences[sync_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the "image available"-semaphore for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const semaphore_t& image_available_semaphore_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mImageAvailableSemaphores[sync_index_for_frame(pCurrentFrameOffset)];
		}
		/** Returns the "render finished"-semaphore for the requested frame.
		*	@param pCurrentFrameOffset	Leave at the default of 0 to request the image index for the current frame.
		*								Specify a negative offset to refer to a previous frame.
		*/
		const auto& render_finished_semaphore_for_frame(int64_t pCurrentFrameOffset = 0) const {
			return mRenderFinishedSemaphores[sync_index_for_frame(pCurrentFrameOffset)];
		}

		/**	Add an extra semaphore to wait on for the given frame id.
		 *	@param	pSemaphore		The semaphore to take ownership for and to set as dependency for a (future) frame.
		 *	@param	pFrameId		The (future) frame-id which this semaphore shall be a dependency for.
		 *							If the parameter is not set, the semaphore will be assigned to the current_frame()-id,
		 *							which means for the next frame which will be rendered. The next frame which will be 
		 *							rendered is the frame with the id current_frame(), assuming this function is called 
		 *							before render_frame() is called.
		 */
		void set_extra_semaphore_dependency(semaphore pSemaphore, std::optional<uint64_t> pFrameId = {});

		void set_one_time_submit_command_buffer(command_buffer pCommandBuffer, std::optional<uint64_t> pFrameId = {});

		std::vector<semaphore> remove_all_extra_semaphore_dependencies_for_frame(uint64_t pFrameId);

		std::vector<command_buffer> remove_all_one_time_submit_command_buffers_for_frame(uint64_t pFrameId);

		void fill_in_extra_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& pSemaphores, uint64_t pFrameId);

		void fill_in_extra_render_finished_semaphores_for_frame(std::vector<vk::Semaphore>& pSemaphores, uint64_t pFrameId);

		//std::vector<semaphore> set_num_extra_semaphores_to_generate_per_frame(uint32_t pNumExtraSemaphores);

		//template<typename CBT, typename... CBTS>
		//void render_frame(CBT pCommandBuffer, CBTS... pCommandBuffers)
		void render_frame(std::vector<std::reference_wrapper<const cgb::command_buffer>> _CommandBufferRefs);

		const auto& renderpass_handle() const { return (*mBackBufferRenderpass).handle(); }

		auto& getrenderpass() const { return mBackBufferRenderpass; }



	protected:
		

#pragma region configuration properties
		// A function which returns the surface format for this window's surface
		std::function<vk::SurfaceFormatKHR(const vk::SurfaceKHR&)> mSurfaceFormatSelector;

		// A function which returns the desired presentation mode for this window's surface
		std::function<vk::PresentModeKHR(const vk::SurfaceKHR&)> mPresentationModeSelector;

		// A function which returns the MSAA sample count for this window's surface
		std::function<vk::SampleCountFlagBits()> mNumberOfSamplesGetter;

		// A function which returns the MSAA state for this window's surface
		std::function<vk::PipelineMultisampleStateCreateInfo()> mMultisampleCreateInfoBuilder;

		// A function which returns the desired number of presentable images in the swap chain
		std::function<uint32_t()> mNumberOfPresentableImagesGetter;

		// A function which returns the number of images which can be rendered into concurrently
		// According to this number, the number of semaphores and fences will be determined.
		std::function<uint32_t()> mNumberOfConcurrentFramesGetter;
#pragma endregion

#pragma region swap chain data for this window surface
		// The frame counter/frame id/frame index/current frame number
		uint64_t mCurrentFrame;

		// The window's surface
		vk::UniqueSurfaceKHR mSurface;
		// The swap chain for this surface
		vk::UniqueSwapchainKHR mSwapChain; 
		// The swap chain's image format
		image_format mSwapChainImageFormat;
		// The swap chain's extent
		vk::Extent2D mSwapChainExtent;
		// Queue family indices which have shared ownership of the swap chain images
		std::vector<uint32_t> mQueueFamilyIndices;
		// Image data of the swap chain images
		vk::ImageCreateInfo mImageCreateInfoSwapChain;
		// All the images of the swap chain
		std::vector<vk::Image> mSwapChainImages; // They don't need to be destroyed explicitely (get...()), ... 
		// All the image views of the swap chain
		std::vector<image_view> mSwapChainImageViews; // ...but the image views do!
#pragma endregion

#pragma region indispensable sync elements
		// Fences to synchronize between frames (CPU-GPU synchronization)
		std::vector<fence> mFences; 
		// Semaphores to wait for an image to become available (GPU-GPU synchronization) // TODO: true?
		std::vector<semaphore> mImageAvailableSemaphores; 
		// Semaphores to wait for rendering to finish (GPU-GPU synchronization) // TODO: true?
		std::vector<semaphore> mRenderFinishedSemaphores; 
#pragma endregion

#pragma region extra sync elements, i.e. exta semaphores
		// Extra semaphores for frames.
		// The first element in the tuple refers to the frame id which is affected.
		// The second element in the is the semaphore to wait on.
		// Extra dependency semaphores will be waited on along with the mImageAvailableSemaphores
		std::vector<std::tuple<uint64_t, semaphore>> mExtraSemaphoreDependencies;
		 
		// Number of extra semaphores to generate per frame upon fininshing the rendering of a frame
		uint32_t mNumExtraRenderFinishedSemaphoresPerFrame;

		// Contains the extra semaphores to be signalled per frame
		// The length of this vector will be: number_of_concurrent_frames() * mNumExtraSemaphoresPerFrame
		// These semaphores will be signalled together with the mRenderFinishedSemaphores
		std::deque<semaphore> mExtraRenderFinishedSemaphores;
#pragma endregion

		// The renderpass used for the back buffers
		renderpass mBackBufferRenderpass;

		// The backbuffers of this window
		std::vector<framebuffer> mBackBuffers;

		// The render pass for this window's UI calls
		vk::RenderPass mUiRenderPass;

		// Command buffers which are only submitted once; taking their ownership, handling their lifetime.
		std::vector<std::tuple<uint64_t, command_buffer>> mOneTimeSubmitCommandBuffers;
	};
}
