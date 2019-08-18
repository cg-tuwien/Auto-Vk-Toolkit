#pragma once

namespace cgb
{
	// ============================== VULKAN CONTEXT ================================
	/**	@brief Context for Vulkan
	 *
	 *	This context abstracts calls to the Vulkan API, for environment-related
	 *	stuff, like window creation etc.,  it relies on GLFW and inherits
	 *	@ref generic_glfw.
	 */
	class vulkan : public generic_glfw
	{
		friend struct texture_handle;
		friend struct image_format;
		friend struct swap_chain_data;
		friend struct shader_handle;
		friend struct pipeline;
		friend class framebuffer_t;
		friend class command_pool;
		friend class command_buffer;
	public:
		
		vulkan();
		vulkan(const vulkan&) = delete;
		vulkan(vulkan&&) = delete;
		vulkan& operator=(const vulkan&) = delete;
		vulkan& operator=(vulkan&&) = delete;
		virtual ~vulkan();

		// Checks a VkResult return type and handles it according to the current Vulkan-Hpp config
		static void check_vk_result(VkResult err);

		template <typename T>
		void track_creation(T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in context::track_creation", typeid(T).name())); }
		template <typename T>
		void track_copy(T*, T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in context::track_copy", typeid(T).name())); }
		template <typename T>
		void track_move(T*, T*)		{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in context::track_move", typeid(T).name())); }
		template <typename T>
		void track_destruction(T*)	{ LOG_WARNING(fmt::format("Encountered unsupported type '{}' in context::track_destruction", typeid(T).name())); }

		vk::Instance& vulkan_instance() { return mInstance; }
		vk::PhysicalDevice& physical_device() { return mPhysicalDevice; }
		vk::Device& logical_device() { return mLogicalDevice; }
		vk::DispatchLoaderDynamic& dynamic_dispatch() { return mDynamicDispatch; }
		uint32_t graphics_queue_index() const { return mGraphicsQueue.queue_index(); }
		uint32_t presentation_queue_index() const { return mPresentQueue.queue_index(); }
		uint32_t transfer_queue_index() const { return mTransferQueue.queue_index(); }
		device_queue& graphics_queue() { return mGraphicsQueue; }
		device_queue& presentation_queue() { return mPresentQueue; }
		device_queue& transfer_queue() { return mTransferQueue; }
		const std::vector<uint32_t>& all_queue_family_indices() { return mDistinctQueueFamilies; }

		/**	Creates a new window, but doesn't open it. Set the window's parameters
		 *	according to your requirements before opening it!
		 *
		 *  @thread_safety This function must only be called from the main thread.
		 */
		window* create_window(const std::string& pTitle);

		void draw_triangle(const graphics_pipeline_t& pPipeline, const command_buffer& pCommandBuffer);

		template <typename Bfr>
		void draw_vertices(const graphics_pipeline& pPipeline, const command_buffer& pCommandBuffer, const Bfr& pVertexBuffer)
		{
			pCommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, pPipeline->handle());
			pCommandBuffer.handle().bindVertexBuffers(0u, { pVertexBuffer.buffer_handle() }, { 0 });
			pCommandBuffer.handle().draw(pVertexBuffer.mVertexCount, 1u, 0u, 0u);                      
		}

		void draw_indexed(const graphics_pipeline& pPipeline, const command_buffer& pCommandBuffer, const vertex_buffer_t& pVertexBuffer, const index_buffer_t& pIndexBuffer)
		{
			pCommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, pPipeline->handle());
			pCommandBuffer.handle().bindVertexBuffers(0u, { pVertexBuffer.buffer_handle() }, { 0 });
			vk::IndexType indexType = to_vk_index_type(pIndexBuffer.meta_data().sizeof_one_element());
			pCommandBuffer.handle().bindIndexBuffer(pIndexBuffer.buffer_handle(), 0u, indexType);
			pCommandBuffer.handle().drawIndexed(pIndexBuffer.meta_data().num_elements(), 1u, 0u, 0u, 0u);
			//pCommandBuffer.handle().drawIndexedIndirect()
		}

		/** Completes all pending work on the device, blocks the current thread until then. */
		void finish_pending_work();

		/** Used to signal the context about the beginning of a composition */
		void begin_composition();

		/** Used to signal the context about the end of a composition */
		void end_composition();

		/** Used to signal the context about the beginning of a new frame */
		void begin_frame();

		/** Used to signal the context about the point within a frame
		 *	when all updates have been performed.
		 *	This usually means that the main thread is about to be awoken
		 *	(but hasn't been yet) in order to start handling input and stuff.
		 */
		void update_stage_done();

		/** Used to signal the context about the end of a frame */
		void end_frame();

	public: // TODO: private
		/** Queries the instance layer properties for validation layers 
		 *  and returns true if a layer with the given name could be found.
		 *  Returns false if not found. 
		 */
		static bool is_validation_layer_supported(const char* pName);

		/**	Compiles all those entries from @ref settings::gValidationLayersToBeActivated into
		 *	an array which are supported by the instance. A warning will be issued for those
		 *	entries which are not supported.
		 */
		static auto assemble_validation_layers();

		/** Create a new vulkan instance with all the application information and
		 *	also set the required instance extensions which GLFW demands.
		 */
		void create_instance();

		/** Method which handles debug callbacks from the validation layers */
		static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

		/** Set up the debug callbacks, i.e. hook into vk to have @ref vk_debug_callback called */
		void setup_vk_debug_callback();

		/** Returns a vector containing all elements from @ref sRequiredDeviceExtensions
		 *  and settings::gRequiredDeviceExtensions
		 */
		static std::vector<const char*> get_all_required_device_extensions();

		/** Checks if the given physical device supports the shading rate image feature
		 */
		static bool supports_shading_rate_image(const vk::PhysicalDevice& device);

		static bool shading_rate_image_extension_requested();
		
		/** Checks whether the given physical device supports all the required extensions,
		 *	namely those stored in @ref settings::gRequiredDeviceExtensions. 
		 *	Returns true if it does, false otherwise.
		 */
		static bool supports_all_required_extensions(const vk::PhysicalDevice& device);

		/** Pick the physical device which looks to be the most promising one */
		void pick_physical_device(vk::SurfaceKHR pSurface);


		/**	Finds all queue families which support certain criteria which are defined by the parameters.
		 *	@param pRequiredFlags	If set, a queue family must support the set flags
		 *	@param pSurface			If set, the queue family must support the given surface
		 *	@return		All which support them are returned as a vector of tuples of indices and data.
		 *				The index is important for further vk-calls and is stored in the first element
		 *				of the tuple, i.e. use @ref std::get<0>() to get the index, @ref std::get<1>() 
		 *				for the data
		 */
		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_queue_families_for_criteria(vk::QueueFlags pRequiredFlags, vk::QueueFlags pForbiddenFlags, std::optional<vk::SurfaceKHR> pSurface);

		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> find_best_queue_family_for(vk::QueueFlags pRequiredFlags, device_queue_selection_strategy pSelectionStrategy, std::optional<vk::SurfaceKHR> pSurface);

		//std::vector<vk::DeviceQueueCreateInfo> compile_create_infos_and_assign_members(
		//	std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> pProps, 
		//	std::vector<std::reference_wrapper<uint32_t>> pAssign);

		/**
		 *
		 */
		void create_and_assign_logical_device(vk::SurfaceKHR pSurface);

		/** Creates the swap chain for the given window and surface with the given parameters
		 *	@param pWindow		[in] The window to create the swap chain for
		 *	@param pSurface		[in] the surface to create the swap chain for
		 *	@param pParams		[in] swap chain creation parameters
		 */
		void create_swap_chain_for_window(window* pWindow);

		/** TODO: TBD */
		vk::RenderPass create_render_pass(image_format pImageFormat, image_format pDepthFormat);

		//pipeline create_ray_tracing_pipeline(
		//	const std::vector<std::tuple<shader_type, shader*>>& pShaderInfos,
		//	const std::vector<vk::DescriptorSetLayout>& pDescriptorSets);

		//std::vector<framebuffer> create_framebuffers(const vk::RenderPass& renderPass, window* pWindow, const image_view_t& pDepthImageView);

		/** Gets a command pool for the given queue family index.
		 *	If the command pool does not exist already, it will be created.
		 */
		command_pool& get_command_pool_for_queue_family(uint32_t pQueueFamilyIndex);
		
		/** Gets a command pool for the given queue's queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	A command pool can be shared for multiple queue families if they have the same queue family index.
		 */
		command_pool& get_command_pool_for_queue(const device_queue& pQueue);
		
		///** Calculates the semaphore index of the current frame */
		//size_t sync_index_curr_frame() const { return mFrameCounter % sActualMaxFramesInFlight; }

		///** Calculates the semaphore index of the previous frame */
		//size_t sync_index_prev_frame() const { return (mFrameCounter - 1) % sActualMaxFramesInFlight; }

		//vk::Semaphore& image_available_semaphore_current_frame() { return mImageAvailableSemaphores[sync_index_curr_frame()]; }

		//vk::Semaphore& render_finished_semaphore_current_frame() { return mRenderFinishedSemaphores[sync_index_curr_frame()]; }

		//vk::Fence& fence_current_frame() { return mInFlightFences[sync_index_curr_frame()]; }

		/** Find (index of) memory with parameters
		 *	@param pMemoryTypeBits		Bit field of the memory types that are suitable for the buffer. [9]
		 *	@param pMemoryProperties	Special features of the memory, like being able to map it so we can write to it from the CPU. [9]
		 */
		uint32_t find_memory_type_index(uint32_t pMemoryTypeBits, vk::MemoryPropertyFlags pMemoryProperties);

		std::shared_ptr<descriptor_pool> get_descriptor_pool_for_layouts(std::initializer_list<std::reference_wrapper<descriptor_set_layout>> pLayouts);

		std::vector<vk::DescriptorSet> create_descriptor_set(std::vector<vk::DescriptorSetLayout> pData);

		bool is_format_supported(vk::Format pFormat, vk::ImageTiling pTiling, vk::FormatFeatureFlags pFormatFeatures);

		vk::PhysicalDeviceRayTracingPropertiesNV get_ray_tracing_properties();

		
	public:
		static std::vector<const char*> sRequiredDeviceExtensions;
		
		vk::Instance mInstance;
		VkDebugUtilsMessengerEXT mDebugCallbackHandle;
		//std::vector<swap_chain_data_ptr> mSurfSwap;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mLogicalDevice;
		vk::DispatchLoaderDynamic mDynamicDispatch;

		device_queue mPresentQueue;
		device_queue mGraphicsQueue;
		device_queue mComputeQueue;
		device_queue mTransferQueue;
		// Vector of queue family indices
		std::vector<uint32_t> mDistinctQueueFamilies;
		// Vector of pairs of queue family indices and queue indices
		std::vector<std::tuple<uint32_t, uint32_t>> mDistinctQueues;

		// Command pools are created/stored per thread and per queue family index.
		// Queue family indices are stored within the command_pool objects, thread indices in the tuple.
		std::deque<std::tuple<std::thread::id, command_pool>> mCommandPools;

		// Descriptor pools are created/stored per thread. 
		// If possible, it is tried to re-use a pool. Even when re-using a pool, it might happen that
		// allocating from it might fail (due to out of memory, for instance). In such cases, a new 
		// pool will be created.
		std::unordered_map<std::thread::id, std::vector<std::weak_ptr<descriptor_pool>>> mDescriptorPools;
	};

	template <>
	inline void vulkan::track_creation<generic_buffer_t>(generic_buffer_t* thing) { LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<uniform_buffer_t>(uniform_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<storage_buffer_t>(storage_buffer_t* thing) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<storage_texel_buffer_t>(storage_texel_buffer_t* thing) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<vertex_buffer_t>(vertex_buffer_t* thing) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<index_buffer_t>(index_buffer_t* thing) { LOG_WARNING("TODO: implement index_buffer_t handling in context::track_creation."); }
	template <>
	inline void vulkan::track_creation<instance_buffer_t>(instance_buffer_t* thing) { LOG_WARNING("TODO: implement instance_buffer_t handling in context::track_creation."); }

	template <>
	inline void vulkan::track_move<generic_buffer_t>(generic_buffer_t* thing, generic_buffer_t* other) { LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<uniform_buffer_t>(uniform_buffer_t* thing, uniform_buffer_t* other) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing, uniform_texel_buffer_t* other) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<storage_buffer_t>(storage_buffer_t* thing, storage_buffer_t* other) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<storage_texel_buffer_t>(storage_texel_buffer_t* thing, storage_texel_buffer_t* other) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<vertex_buffer_t>(vertex_buffer_t* thing, vertex_buffer_t* other) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<index_buffer_t>(index_buffer_t* thing, index_buffer_t* other) { LOG_WARNING("TODO: implement index_buffer_t handling in context::track_move."); }
	template <>
	inline void vulkan::track_move<instance_buffer_t>(instance_buffer_t* thing, instance_buffer_t* other) { LOG_WARNING("TODO: implement instance_buffer_t handling in context::track_move."); }

	template <>
	inline void vulkan::track_destruction<generic_buffer_t>(generic_buffer_t* thing) {
		LOG_WARNING("TODO: implement generic_buffer_t handling in context::track_destruction."); 
	}
	template <>
	inline void vulkan::track_destruction<uniform_buffer_t>(uniform_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_buffer_t handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<uniform_texel_buffer_t>(uniform_texel_buffer_t* thing) { LOG_WARNING("TODO: implement uniform_texel_buffer_t handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<storage_buffer_t>(storage_buffer_t* thing) { LOG_WARNING("TODO: implement storage_buffer_t handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<storage_texel_buffer_t>(storage_texel_buffer_t* thing) { LOG_WARNING("TODO: implement storage_texel_buffer_t handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<vertex_buffer_t>(vertex_buffer_t* thing) { LOG_WARNING("TODO: implement vertex_buffer_t handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<index_buffer_t>(index_buffer_t* thing) { LOG_WARNING("TODO: implement buffer_t<index_buffer_data> handling in context::track_destruction."); }
	template <>
	inline void vulkan::track_destruction<instance_buffer_t>(instance_buffer_t* thing) { LOG_WARNING("TODO: implement buffer_t<instance_buffer_t> handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<shader>(shader* thing)				{ LOG_WARNING("TODO: implement 'shader' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<shader>(shader* thing, shader* other)	{ LOG_WARNING("TODO: implement 'shader' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<shader>(shader* thing)			{ LOG_WARNING("TODO: implement 'shader' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<acceleration_structure>(acceleration_structure* thing)		{ LOG_WARNING("TODO: implement 'acceleration_structure' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<acceleration_structure>(acceleration_structure* thing, acceleration_structure* other)			{ LOG_WARNING("TODO: implement 'acceleration_structure' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<acceleration_structure>(acceleration_structure* thing)	{ LOG_WARNING("TODO: implement 'acceleration_structure' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<sampler_t>(sampler_t* thing)		{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<sampler_t>(sampler_t* thing, sampler_t* other)			{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<sampler_t>(sampler_t* thing)	{ LOG_WARNING("TODO: implement 'sampler_t' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<image_view_t>(image_view_t* thing)		{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<image_view_t>(image_view_t* thing, image_view_t* other)			{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<image_view_t>(image_view_t* thing)	{ LOG_WARNING("TODO: implement 'image_view_t' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<renderpass_t>(renderpass_t* thing)		{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<renderpass_t>(renderpass_t* thing, renderpass_t* other)			{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<renderpass_t>(renderpass_t* thing)	{ LOG_WARNING("TODO: implement 'renderpass_t' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<graphics_pipeline_t>(graphics_pipeline_t* thing)		{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<graphics_pipeline_t>(graphics_pipeline_t* thing, graphics_pipeline_t* other)			{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<graphics_pipeline_t>(graphics_pipeline_t* thing)	{ LOG_WARNING("TODO: implement 'graphics_pipeline_t' handling in context::track_destruction."); }

	template <>
	inline void vulkan::track_creation<framebuffer_t>(framebuffer_t* thing)		{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_creation."); }
	template <>
	inline void vulkan::track_move<framebuffer_t>(framebuffer_t* thing, framebuffer_t* other)			{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_move."); }
	template <>
	inline void vulkan::track_destruction<framebuffer_t>(framebuffer_t* thing)	{ LOG_WARNING("TODO: implement 'framebuffer_t' handling in context::track_destruction."); }


	// [1] Vulkan Tutorial, Depth buffering, https://vulkan-tutorial.com/Depth_buffering
}
