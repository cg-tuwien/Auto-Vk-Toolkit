#pragma once
#include "window_base.hpp"

namespace avk
{
	class window : public window_base
	{
		friend class context_generic_glfw;
		friend class context_vulkan;
	public:

		/**
		*	this struct is used to distinguish between updating
		*	an existing swap chain, vs. creating a new one initially.
		*/
		enum struct swapchain_creation_mode
		{
			update_existing_swapchain,
			create_new_swapchain
		};

		using frame_id_t = int64_t;
		using outdated_swapchain_t = std::tuple<vk::UniqueHandle<vk::SwapchainKHR, DISPATCH_LOADER_CORE_TYPE>, std::vector<avk::image_view>, avk::renderpass, std::vector<avk::framebuffer>>;
		
		using any_window_resource_t = std::variant<
			// Those from any_owning_resorce_t:
		    bottom_level_acceleration_structure,
		    buffer,
		    buffer_view,
		    command_buffer,
		    command_pool,
		    compute_pipeline,
		    fence,
		    framebuffer,
		    graphics_pipeline,
		    image,
		    image_sampler,
		    image_view,
		    query_pool,
		    ray_tracing_pipeline,
		    renderpass,
		    sampler,
		    semaphore,
		    top_level_acceleration_structure,
		    // ...plus the "outdated swapchain" ones:
            vk::UniqueHandle<vk::SwapchainKHR, DISPATCH_LOADER_CORE_TYPE>,
	        std::vector<avk::image_view>,
	        std::vector<avk::framebuffer>,
	        outdated_swapchain_t
	    >;

		// A mutex used to protect concurrent command buffer submission
		static std::mutex sSubmitMutex;

		window() = default;
		window(window&&) noexcept = default;
		window(const window&) = delete;
		window& operator =(window&&) noexcept = default;
		window& operator =(const window&) = delete;
		~window()
		{
			mCurrentFrameFinishedFence.reset();
			mCurrentFrameImageAvailableSemaphore.reset();
			mLifetimeHandledResources.clear();
			mPresentSemaphoreDependencies.clear();
			mImageAvailableSemaphores.clear();
			mFramesInFlightFences.clear();
			mSwapChainImageViews.clear();
		}

		/** Set to true to create a resizable window, set to false to prevent the window from being resized */
		void enable_resizing(bool aEnable);

		/** Request a framebuffer for this window which is capable of sRGB formats */
		void request_srgb_framebuffer(bool aRequestSrgb);

		/** Sets the presentation mode for this window's swap chain. */
		void set_presentaton_mode(avk::presentation_mode aMode);

		/** Sets the number of presentable images for a swap chain */
		void set_number_of_presentable_images(uint32_t aNumImages);

		/** Sets the number of images which can be rendered into concurrently,
		 *	i.e. the number of "frames in flight"
		 */
		void set_number_of_concurrent_frames(frame_id_t aNumConcurrent);

		/** Sets additional attachments which shall be added to the back buffer
		 *	in addition to the obligatory color attachment.
		 */
		void set_additional_back_buffer_attachments(std::vector<avk::attachment> aAdditionalAttachments);

		/** Sets the image usage properties that shall be used for creation of the swap chain images
		 */
		void set_image_usage_properties(avk::image_usage aImageUsageProperties);

		/** Gets the image usage properties that have been set, or sensible default values
		 */
		avk::image_usage get_config_image_usage_properties();
		
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

		/** Get the minimum number of concurrent/presentable images for a swap chain.
		*	If no value is set, the surfaces minimum number + 1 will be returned.
		*/
		uint32_t get_config_number_of_presentable_images();

		/** Get the number of concurrent frames.
		*	If no value is explicitely set, the same number as the number of presentable images will be returned.
		*/
		frame_id_t get_config_number_of_concurrent_frames();

		/**	Gets the descriptions of the additional back buffer attachments
		 */
		std::vector<avk::attachment> get_additional_back_buffer_attachments();


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
		[[nodiscard]] const avk::image_t& swap_chain_image_reference_at_index(size_t aIdx) const {
			return mSwapChainImageViews[aIdx]->get_image();
		}
		/** Gets a collection containing all this window's swap chain image views. */
		[[nodiscard]] std::vector<std::reference_wrapper<const avk::image_t>> swap_chain_image_views() const {
			std::vector<std::reference_wrapper<const avk::image_t>> allImageViews;
			allImageViews.reserve(mSwapChainImageViews.size());
			for (auto& imView : mSwapChainImageViews) {
				allImageViews.push_back(std::ref(imView->get_image())); // TODO: Would it be better to include the owning_resource in the resource_reference?
			}
			return allImageViews;
		}
		/** Gets this window's swap chain's image view at the specified index. */
		[[nodiscard]] avk::image_view swap_chain_image_view_at_index(size_t aIdx) const {
			return mSwapChainImageViews[aIdx];
		}
		/** Gets reference to the window's swap chain's image view at the specified index. */
		[[nodiscard]] const avk::image_view_t& swap_chain_image_view_reference_at_index(size_t aIdx) const {
			return mSwapChainImageViews[aIdx].get();
		}

		/** Gets a collection containing all this window's back buffers. */
		[[nodiscard]] std::vector<avk::framebuffer> backbuffers() {
			std::vector<avk::framebuffer> allFramebuffers;
			allFramebuffers.reserve(mBackBuffers.size());
			for (auto& fb : mBackBuffers) {
				allFramebuffers.push_back(fb);
			}
			return allFramebuffers;
		}

		/** Gets this window's back buffer at the specified index. */
		[[nodiscard]] const avk::framebuffer_t& backbuffer_reference_at_index(size_t aIdx) const {
			return *mBackBuffers[aIdx];
		}

		/** Gets this window's back buffer at the specified index. */
		[[nodiscard]] avk::framebuffer backbuffer_at_index(size_t aIdx) {
			return mBackBuffers[aIdx];
		}

		/** Gets the number of how many frames are (potentially) concurrently rendered into,
		 *	or put differently: How many frames are (potentially) "in flight" at the same time.
		 */
		[[nodiscard]] frame_id_t number_of_frames_in_flight() const {
			return static_cast<frame_id_t>(mFramesInFlightFences.size());
		}

		/** Gets the number of images there are in the swap chain */
		[[nodiscard]] size_t number_of_swapchain_images() const {
			return mSwapChainImageViews.size();
		}

		/** Gets the current frame index. */
		[[nodiscard]] frame_id_t current_frame() const {
			return mCurrentFrame;
		}

		/** Returns the "in flight index" for the requested frame.
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		[[nodiscard]] frame_id_t in_flight_index_for_frame(std::optional<frame_id_t> aFrameId = {}) const {
			return aFrameId.value_or(current_frame()) % number_of_frames_in_flight();
		}
		/** Returns the "in flight index" for the requested frame. */
		[[nodiscard]] frame_id_t current_in_flight_index() const {
			return current_frame() % number_of_frames_in_flight();
		}

		/** Returns the "swapchain image index" for the current frame.  */
		[[nodiscard]] auto current_image_index() const {
			return mCurrentFrameImageIndex;
		}
		/** Returns the "swapchain image index" for the previous frame.  */
		[[nodiscard]] auto previous_image_index() const {
			return mPreviousFrameImageIndex;
		}

		/** Returns the backbuffer for the current frame */
		[[nodiscard]] avk::framebuffer current_backbuffer() const {
			return mBackBuffers[current_image_index()];
		}
		/** Returns the backbuffer for the previous frame */
		[[nodiscard]] avk::framebuffer previous_backbuffer() const {
			return mBackBuffers[previous_image_index()];
		}

		/** Returns the backbuffer for the current frame */
		[[nodiscard]] const avk::framebuffer_t& current_backbuffer_reference() const {
			return mBackBuffers[current_image_index()].get();
		}
		/** Returns the backbuffer for the previous frame */
		[[nodiscard]] const avk::framebuffer_t& previous_backbuffer_reference() const {
			return mBackBuffers[previous_image_index()].get();
		}

		/** Returns the swap chain image for the current frame. */
		[[nodiscard]] const avk::image_t& current_image_reference() const {
			return swap_chain_image_reference_at_index(current_image_index());
		}
		/** Returns the swap chain image for the previous frame. */
		[[nodiscard]] const avk::image_t& previous_image_reference() const {
			return swap_chain_image_reference_at_index(previous_image_index());
		}

		/** Returns the swap chain image view for the current frame. */
		[[nodiscard]] avk::image_view current_image_view() {
			return mSwapChainImageViews[current_image_index()];
		}
		/** Returns the swap chain image view for the previous frame. */
		[[nodiscard]] avk::image_view previous_image_view() {
			return mSwapChainImageViews[previous_image_index()];
		}

		/** Returns the swap chain image view for the current frame. */
		[[nodiscard]] const avk::image_view_t& current_image_view_reference() {
			return mSwapChainImageViews[current_image_index()].get();
		}
		/** Returns the swap chain image view for the previous frame. */
		[[nodiscard]] const avk::image_view_t& previous_image_view_reference() {
			return mSwapChainImageViews[previous_image_index()].get();
		}

		/** Returns the fence for the requested frame, which depends on the frame's "in flight index".
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		[[nodiscard]] avk::fence fence_for_frame(std::optional<frame_id_t> aFrameId = {}) {
			return mFramesInFlightFences[in_flight_index_for_frame(aFrameId)];
		}
		/** Returns the fence for the current frame. */
		[[nodiscard]] avk::fence current_fence() {
			return mFramesInFlightFences[current_in_flight_index()];
		}

		/** Returns the "image available"-semaphore for the requested frame, which depends on the frame's "in flight index".
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		avk::semaphore image_available_semaphore_for_frame(std::optional<frame_id_t> aFrameId = {}) {
			return mImageAvailableSemaphores[in_flight_index_for_frame(aFrameId)];
		}
		/** Returns the "image available"-semaphore for the current frame. */
		avk::semaphore current_image_available_semaphore() {
			return mImageAvailableSemaphores[current_in_flight_index()];
		}

		/** Adds the given semaphore as an additional present-dependency to the given frame-id.
		 *	That means, before an image is handed over to the presentation engine, the given semaphore must be signaled.
		 *	You can add multiple render finished semaphores, but there should (must!) be at least one per frame.
		 *	Important: It is the responsibility of the CALLER to ensure that the semaphore will be signaled.
		 *
		 *	@param aFrameId		If set, refers to the absolute frame-id of a specific frame.
		 *						If not set, refers to the current frame, i.e. `current_frame()`.
		 */
		void add_present_dependency_for_frame(avk::semaphore aSemaphore, std::optional<frame_id_t> aFrameId = {}) {
			mPresentSemaphoreDependencies.emplace_back(aFrameId.value_or(current_frame()), std::move(aSemaphore));
		}
		/** Adds the given semaphore as an additional present-dependency to the current frame.
		 *	That means, before an image is handed over to the presentation engine, the given semaphore must be signaled.
		 *	You can add multiple render finished semaphores, but there should (must!) be at least one per frame.
		 *	Important: It is the responsibility of the CALLER to ensure that the semaphore will be signaled.
		 */
		void add_present_dependency_for_current_frame(avk::semaphore aSemaphore) {
			mPresentSemaphoreDependencies.emplace_back(current_frame(), std::move(aSemaphore));
		}

		/**	Pass a "single use" command buffer for the given frame and have its lifetime handled.
		 *	The submitted command buffer's commands have an execution dependency on the back buffer's
		 *	image to become available.
		 *	Put differently: No commands will execute until the referenced frame's swapchain image has become available.
		 *	@param	aCommandBuffer	The command buffer to take ownership of and to handle lifetime of.
		 *	@param	aFrameId		The frame this command buffer is associated to.
		 */
		//void handle_lifetime(avk::command_buffer aCommandBuffer, std::optional<frame_id_t> aFrameId = {});
	    void handle_lifetime(any_window_resource_t aResource, std::optional<frame_id_t> aFrameId = {});

		///** Pass the resources of an outdated swap chain and have its lifetime automatically handled.
		// *	@param aOutdatedSwapchain	Resources to be moved into this function and to be lifetime-handled
		// *	@param aFrameId				The frame these resources are associated with. They will be deleted
		// *								in frame #current_frame() + number_number_of_frames_in_flight().
		// */
		//void handle_lifetime(outdated_swapchain_resource_t&& aOutdatedSwapchain, std::optional<frame_id_t> aFrameId = {});


		/**	Remove all the semaphores which were dependencies for one of the previous frames, but
		 *	can now be safely destroyed.
		 */
		std::vector<avk::semaphore> remove_all_present_semaphore_dependencies_for_frame(frame_id_t aPresentFrameId);

		/** Remove all the resources (such as "single use" command buffers) for the given frame.
		 *	The resources are moved out of the internal storage and returned to the caller.
		 */
        std::vector<any_window_resource_t> clean_up_resources_for_frame(frame_id_t aPresentFrameId);

		///** Remove all the outdated swap chain resources which are safe to be removed in this frame. */
		//void clean_up_outdated_swapchain_resources_for_frame(frame_id_t aPresentFrameId);

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
		auto renderpass_handle() const { return mBackBufferRenderpass->handle(); }

		/** Gets a const reference to the backbuffer's render pass
		 */
		[[nodiscard]] const avk::renderpass_t& renderpass_reference() const { return mBackBufferRenderpass.get(); }

		/** Gets the backbuffer's render pass
		 */
		[[nodiscard]] avk::renderpass get_renderpass() const { return mBackBufferRenderpass; }

		/**	This is intended to be used as a command buffer lifetime handler for `avk::old_sync::with_barriers`.
		 *	The specified frame id is the frame where the command buffer has to be guaranteed to finish
		 *	(be it by the means of pipeline barriers, semaphores, or fences) and hence, it will be destroyed
		 *	either number_of_in_flight_frames() or number_of_swapchain_images() frames later.
		 *
		 *	Example usage:
		 *	avk::old_sync::with_barriers(avk::context().main_window().handle_command_buffer_lifetime());
		 */
		auto command_buffer_lifetime_handler(std::optional<frame_id_t> aFrameId = {})
		{
			return [this, aFrameId](avk::command_buffer aCommandBufferToLifetimeHandle){
				handle_lifetime(std::move(aCommandBufferToLifetimeHandle), aFrameId);
			};
		}

		/** Set the queue families which will have shared ownership of the of the swap chain images. */
		void set_queue_family_ownership(std::vector<uint32_t> aQueueFamilies);

		/** Set the queue family which will have exclusive ownership of the of the swap chain images. */
		void set_queue_family_ownership(uint32_t aQueueFamily);

		/** Set the queue that shall handle presenting. You MUST set it if you want to show any rendered images in this window!
		 *	Should you use this method to change the presentation queue in a running application, please note that the
		 *	swap chain will NOT be recreated by invoking this method---this might, or might not be the result that you're after.
		 *	Should you desire to recreate the swapchain, too, e.g., to switch the ownership of the images, then
		 *	please first use window::set_queue_family_ownership, then call window::set_present_queue! 
		 */
		void set_present_queue(avk::queue& aPresentQueue);

		/** Returns whether or not the current frame's image available semaphore has already been consumed (by user code),
		 *	which must happen once in every frame!
		 */
		bool has_consumed_current_image_available_semaphore() const {
			return !mCurrentFrameImageAvailableSemaphore.has_value();
		}

		/** Get a reference to the image available semaphore of the current frame.
		 *	It must be used by user code for a semaphore-wait operation, at a useful location, 
		 *	where the swapchain image must have become available for being rendered into.
		 */
		avk::semaphore consume_current_image_available_semaphore() {
			if (!mCurrentFrameImageAvailableSemaphore.has_value()) {
				throw avk::runtime_error("Current frame's image available semaphore has already been consumed. Must be consumed EXACTLY once. Do not try to get it multiple times!");
			}
			auto instance = std::move(mCurrentFrameImageAvailableSemaphore.value());
			mCurrentFrameImageAvailableSemaphore.reset();
			return instance;
		}

		/** Returns whether or not the current frame's render finished fence has already been retrieved (by user code)
		 *	for being used in a fence-signal operation, which must happen once in every frame!
		 */
		bool has_used_current_frame_finished_fence() const {
			return !mCurrentFrameFinishedFence.has_value();
		}

		/** Get a reference to the current frame's render finished fence. 
		 *	It must be used by user code for a fence-signal operation, indicating when a frame has been rendered completely.
		 */
		avk::fence use_current_frame_finished_fence() {
			if (!mCurrentFrameFinishedFence.has_value()) {
				throw avk::runtime_error("Current frame's frame finished fence has already been used. Must be used EXACTLY once. Do not try to get it multiple times!");
			}
			auto instance = std::move(mCurrentFrameFinishedFence.value());
			mCurrentFrameFinishedFence.reset();
			return instance;
		}


		/**
		 * create or update the swap chain (along with auxiliary resources) for this window depending on
		 * the aCreationMode. Additionally, backbuffers are also created or updated.
		 *
		 * @param aCreationMode inidicates whether the swap chain is being created for the first time,
		 * or whether the existing swapchain has to be updated
		 */
		void create_swap_chain(swapchain_creation_mode aCreationMode);

		/**
		 * updates resolution and forwards call to create_swap_chain to recreate the swap chain
		 */
		void update_resolution_and_recreate_swap_chain();

		/**	Returns a vector of commands containing layout transitions for each one of the attachments of each of the backbuffer framebuffers into their respective final layouts.
		 *	For color attachments, this will most likely result in a layout transition from undefined >> color_attachment_optimal.
		 *	For depth/stencil attachments, this will most likely result in a layout transition from undefined >> depth_stencil_optimal.
		 */
		std::vector<avk::recorded_commands_t> layout_transitions_for_all_backbuffer_images();

	private:
		/** Invoked internally to update the presentation queue should a new one have been set
		 */
		void update_active_presentation_queue();

		/**
		 * constructs or updates ImageCreateInfo and SwapChainCreateInfo before the swap chain is
		 * created.
		 *
		 * @param aCreationMode indicates whether to create or just update the existing constructs.
		 */
		void construct_swap_chain_creation_info(swapchain_creation_mode aCreationMode);

		/**
		 * constructs or updates window backbuffers after the swap chain has been created or modified.
		 *
		 * @param aCreationMode indicates whether to just update or create the backbuffers from the ground up.
		 */
		void construct_backbuffers(swapchain_creation_mode aCreationMode);

		/** Update the number of fences and semaphores which handle concurrent frames synchronization
		 *  @param aCreationMode indicates whether a new creation or an update is needed.
	     */
		void update_concurrent_frame_synchronization(swapchain_creation_mode aCreationMode);


		// Helper method used in sync_before_render().
		// If the swap chain must be re-created, it will recursively invoke itself.
		void acquire_next_swap_chain_image_and_prepare_semaphores();

		// Helper method that fills the given 2 vectors with the present semaphore dependencies for the given frame-id
		void fill_in_present_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, frame_id_t aFrameId) const;


#pragma region configuration properties
		// A function which returns whether or not the window should be resizable
		bool mShallBeResizable = false;

		// A function which returns the surface format for this window's surface
		avk::unique_function<vk::SurfaceFormatKHR(const vk::SurfaceKHR&)> mSurfaceFormatSelector;

		// A function which returns the desired presentation mode for this window's surface
		avk::unique_function<vk::PresentModeKHR(const vk::SurfaceKHR&)> mPresentationModeSelector;

		// A function which returns the desired number of presentable images in the swap chain
		avk::unique_function<uint32_t()> mNumberOfPresentableImagesGetter;

		// A function which returns the number of images which can be rendered into concurrently
		// According to this number, the number of semaphores and fences will be determined.
		avk::unique_function<frame_id_t()> mNumberOfConcurrentFramesGetter;

		// A function which returns attachments which shall be attached to the back buffer
		// in addition to the obligatory color attachment.
		avk::unique_function<std::vector<avk::attachment>()> mAdditionalBackBufferAttachmentsGetter;
#pragma endregion

#pragma region swap chain data for this window surface
		// The frame counter/frame id/frame index/current frame number
		frame_id_t mCurrentFrame;
		
		// The window's surface
		vk::UniqueHandle<vk::SurfaceKHR, DISPATCH_LOADER_CORE_TYPE> mSurface;
		// The swap chain create info
		vk::SwapchainCreateInfoKHR mSwapChainCreateInfo;
		// The swap chain for this surface
		vk::UniqueHandle<vk::SwapchainKHR, DISPATCH_LOADER_CORE_TYPE> mSwapChain;
		// The swap chain's image format
		vk::Format mSwapChainImageFormat;
		// The swap chain's color space
		vk::ColorSpaceKHR mSwapChainColorSpace;
		// The swap chain's extent
		vk::Extent2D mSwapChainExtent;
		// Queue family indices which have shared ownership of the swap chain images
		avk::unique_function<std::vector<uint32_t>()> mQueueFamilyIndicesGetter;
		// Image data of the swap chain images
		vk::ImageCreateInfo mImageCreateInfoSwapChain;
		// All the image views of the swap chain
		std::vector<avk::image_view> mSwapChainImageViews; // ...but the image views do!

		// A function which returns the image usage properties for the back buffer images
		avk::unique_function<avk::image_usage()> mImageUsageGetter;

#pragma endregion

#pragma region indispensable sync elements
		// Fences to synchronize between frames
		std::vector<avk::fence> mFramesInFlightFences;
		// Semaphores to wait for an image to become available
		std::vector<avk::semaphore> mImageAvailableSemaphores;
		// Fences to make sure that no two images are written into concurrently
		std::vector<int> mImagesInFlightFenceIndices;

		// Semaphores to be waited on before presenting the image PER FRAME.
		// The first element in the tuple refers to the frame id which is affected.
		// The second element in the is the semaphore to wait on.
		std::vector<std::tuple<frame_id_t, avk::semaphore>> mPresentSemaphoreDependencies;
#pragma endregion

		// The renderpass used for the back buffers
		avk::renderpass mBackBufferRenderpass;

		// The backbuffers of this window
		std::vector<avk::framebuffer> mBackBuffers;

		// The render pass for this window's UI calls
		vk::RenderPass mUiRenderPass;

		// This container handles resource lifetimes, such as (single use) command buffers.
		// A such-handled resource's lifetime lasts until the specified int64_t frame-id + number_of_in_flight_frames()
		// This container also handles old swap chain resources which are to be deleted at a certain future frame
		std::deque<std::tuple<frame_id_t, any_window_resource_t>> mLifetimeHandledResources;
		
		// The queue that is used for presenting. It MUST be set to a valid queue if window::render_frame() is ever going to be invoked.
		avk::unique_function<avk::queue*()> mPresentationQueueGetter;

		// The presentation queue that is active
		avk::queue* mActivePresentationQueue = nullptr;

		// Current frame's image index
		uint32_t mCurrentFrameImageIndex;
		// Previous frame's image index
		uint32_t mPreviousFrameImageIndex;

		// Must be consumed EXACTLY ONCE per frame:
		std::optional<avk::semaphore> mCurrentFrameImageAvailableSemaphore;

		// Must be used EXACTLY ONCE per frame:
		std::optional<avk::fence> mCurrentFrameFinishedFence;
		
#pragma region recreation management
		struct recreation_determinator
		{
			/** Formulate the reason why resource recreation has become necessary.
			 */
			enum struct reason
			{
				suboptimal_swap_chain,                      // Indicates that 'vk::Result::eSuboptimalKHR' has been returned as the result of acquireNextImageKHR or presentKHR.
				invalid_swap_chain,                         // Indicates that the swapchain is out of date or otherwise in an invalid state.
				backbuffer_attachments_changed,             // Indicates that the additional attachments of the backbuffer have changed.
				presentation_mode_changed,                  // Indicates that the presentation mode has changed.
				concurrent_frames_count_changed,            // Indicates that the number of concurrent frames has been modified.
				presentable_images_count_changed,           // Indicates that the number of presentable images has been modified.
				image_format_or_properties_changed          // Indicates that the image format of the framebuffer has been modified.
			};
			/** Reset the state of the determinator. This should be done per frame once the required changes have been applied. */
			void reset() { mInvalidatedProperties.reset(); }

			/** Signal that recreation is potentially required and state the reason.
			 *  @param aReason the reason for the recreation
			 */
			void set_recreation_required_for(reason aReason)      { mInvalidatedProperties[static_cast<size_t>(aReason)] = true; }

			/** Probe whether recreation is required due to a specific reason.
			 *  @param aReason the reason for the recreation
			 *  @return true if the bit specified by aReason has been set to 1.
			 */
			bool is_recreation_required_for(reason aReason) const { return mInvalidatedProperties.test(static_cast<size_t>(aReason)); }

			/** Check whether any recreation is necessary.
			 *  This is a useful before more specific conditions are probed.
			 *  @return true if any of the bits have been set to 1.
			 */
			bool is_any_recreation_necessary()              const { return mInvalidatedProperties.any(); }

			/** Check whether a change to number of concurrent frames is the only reason for requiring recreation.
			 *  @return true if 'concurrent_frames_count_changed' is the only bit set to 1.
			 */
			bool has_concurrent_frames_count_changed_only() const { return mInvalidatedProperties.test(static_cast<size_t>(reason::concurrent_frames_count_changed)) && mInvalidatedProperties.count() == 1; }

			/** Check whether swapchain requires recreation.
			 *  @return true if any of the bits other than 'concurrent_frames_count_changed' are set to 1.
			 */
			bool is_swapchain_recreation_necessary()        const { return is_any_recreation_necessary() && !has_concurrent_frames_count_changed_only(); }
		private:
			std::bitset<8> mInvalidatedProperties = 0; // increase the length of bitset if more reasons are added.
		} mResourceRecreationDeterminator;
#pragma endregion
	};

}
