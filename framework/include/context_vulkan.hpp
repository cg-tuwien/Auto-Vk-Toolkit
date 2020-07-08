#pragma once

namespace xk
{
	struct pool_id
	{
		std::thread::id mThreadId;
		int mName;
	};

	static bool operator==(const pool_id& left, const pool_id& right)
	{
		return left.mThreadId == right.mThreadId && left.mName == right.mName;
	}

	static bool operator!=(const pool_id& left, const pool_id& right)
	{
		return !(left==right);
	}
}

namespace std // Inject hash for `cgb::sampler_t` into std::
{
	template<> struct hash<xk::pool_id>
	{
		std::size_t operator()(xk::pool_id const& o) const noexcept
		{
			std::size_t h = 0;
			ak::hash_combine(h, o.mThreadId, o.mName);
			return h;
		}
	};
}

namespace xk
{	
	// ============================== VULKAN CONTEXT ================================
	/**	@brief Context for Vulkan
	 *
	 *	This context abstracts calls to the Vulkan API, for environment-related
	 *	stuff, like window creation etc.,  it relies on GLFW and inherits
	 *	@ref generic_glfw.
	 */
	class vulkan : public generic_glfw, public ak::root
	{
	public:
		vulkan();
		vulkan(const vulkan&) = delete;
		vulkan(vulkan&&) = delete;
		vulkan& operator=(const vulkan&) = delete;
		vulkan& operator=(vulkan&&) = delete;
		~vulkan();

		// Checks a VkResult return type and handles it according to the current Vulkan-Hpp config
		static void check_vk_result(VkResult err);
		
		vk::Instance vulkan_instance() { return mInstance; }
		vk::PhysicalDevice physical_device() override { return mPhysicalDevice; }
		vk::Device device() override { return mLogicalDevice; }
		vk::DispatchLoaderDynamic dynamic_dispatch() override { return mDynamicDispatch; }

		uint32_t graphics_queue_index() const { return mGraphicsQueue.queue_index(); }
		uint32_t presentation_queue_index() const { return mPresentQueue.queue_index(); }
		uint32_t transfer_queue_index() const { return mTransferQueue.queue_index(); }

		ak::queue& graphics_queue() { return mGraphicsQueue; }
		ak::queue& presentation_queue() { return mPresentQueue; }
		ak::queue& transfer_queue() { return mTransferQueue; }
		
		const std::vector<uint32_t>& all_queue_family_indices() { return mDistinctQueueFamilies; }

		/** Gets a command pool for the given queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 */
		ak::command_pool& get_command_pool_for(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aFlags);
		
		/** Gets a command pool for the given queue's queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	A command pool can be shared for multiple queue families if they have the same queue family index.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 */
		ak::command_pool& get_command_pool_for(const ak::queue& aQueue, vk::CommandPoolCreateFlags aFlags);
		
		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		ak::command_buffer create_command_buffer(bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a "standard" command buffer which is not necessarily short-lived
		 *	and can be re-submitted, but not necessarily re-recorded.
		 *
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		std::vector<ak::command_buffer> create_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 */
		ak::command_buffer create_single_use_command_buffer(bool aPrimary = true);

		/** Creates a command buffer which is intended to be used as a one time submit command buffer
		 *	@param	aNumBuffers					How many command buffers to be created.
		 */
		std::vector<ak::command_buffer> create_single_use_command_buffers(uint32_t aNumBuffers, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		ak::command_buffer create_resettable_command_buffer(bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/** Creates a command buffer which is intended to be reset (and possible re-recorded).
		 *	@param	aNumBuffers					How many command buffers to be created.
		 *	@param	aSimultaneousUseEnabled		`true` means that the command buffer to be created can be 
		 *										resubmitted to a queue while it is in the pending state.
		 *										It also means that it can be recorded into multiple primary
		 *										command buffers, if it is intended to be used as a secondary.
		 */
		std::vector<ak::command_buffer> create_resettable_command_buffers(uint32_t aNumBuffers, bool aSimultaneousUseEnabled = false, bool aPrimary = true);

		/**	Creates a new window, but doesn't open it. Set the window's parameters
		 *	according to your requirements before opening it!
		 *
		 *  @thread_safety This function must only be called from the main thread.
		 */
		window* create_window(const std::string& _Title);

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
		static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_utils_callback(VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT*, void*);

		/** Set up the debug callbacks, i.e. hook into vk to have @ref vk_debug_callback called */
		void setup_vk_debug_callback();

		/** Method which handles debug report callbacks from the VK_EXT_debug_report extension */
		static VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_report_callback(VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char*, const char*, void*);

		/** Setup debug report callbacks from VK_EXT_debug_report */
		void setup_vk_debug_report_callback();

		/** Returns a vector containing all elements from @ref sRequiredDeviceExtensions
		 *  and settings::gRequiredDeviceExtensions
		 */
		static std::vector<const char*> get_all_required_device_extensions();

		/** Checks if the given physical device supports the shading rate image feature
		 */
		static bool supports_shading_rate_image(const vk::PhysicalDevice& device);

		static bool shading_rate_image_extension_requested();
		static bool ray_tracing_extension_requested();
		
		/** Checks whether the given physical device supports all the required extensions,
		 *	namely those stored in @ref settings::gRequiredDeviceExtensions. 
		 *	Returns true if it does, false otherwise.
		 */
		static bool supports_all_required_extensions(const vk::PhysicalDevice& device);

		/** Pick the physical device which looks to be the most promising one */
		void pick_physical_device(vk::SurfaceKHR pSurface);

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

		//pipeline create_ray_tracing_pipeline(
		//	const std::vector<std::tuple<shader_type, shader*>>& pShaderInfos,
		//	const std::vector<vk::DescriptorSetLayout>& pDescriptorSets);

		//std::vector<framebuffer> create_framebuffers(const vk::RenderPass& renderPass, window* pWindow, const image_view_t& pDepthImageView);

		ak::descriptor_cache_interface* get_standard_descriptor_cache() { return &mStandardDescriptorCache; }

		auto requested_physical_device_features() const { return mRequestedPhysicalDeviceFeatures; }
		void set_requested_physical_device_features(vk::PhysicalDeviceFeatures aNewValue) { mRequestedPhysicalDeviceFeatures = aNewValue; }
		auto requested_vulkan12_device_features() const { return mRequestedVulkan12DeviceFeatures; }
		void set_requested_vulkan12_device_features(vk::PhysicalDeviceVulkan12Features aNewValue) { mRequestedVulkan12DeviceFeatures = aNewValue; }
		
	public:
		static std::vector<const char*> sRequiredDeviceExtensions;

		// A mutex which protects the vulkan context from concurrent access from different threads
		// e.g. during parallel recording of command buffers.
		// It is only used in some functions, which are expected to be called during runtime (i.e.
		// during `cg_element::update` or cg_element::render` calls), not during initialization.
		static std::mutex sConcurrentAccessMutex;
		
		vk::Instance mInstance;
		VkDebugUtilsMessengerEXT mDebugUtilsCallbackHandle;
		VkDebugReportCallbackEXT mDebugReportCallbackHandle;
		//std::vector<swap_chain_data_ptr> mSurfSwap;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mLogicalDevice;
		vk::DispatchLoaderDynamic mDynamicDispatch;

		ak::queue mPresentQueue;
		ak::queue mGraphicsQueue;
		ak::queue mComputeQueue;
		ak::queue mTransferQueue;
		// Vector of queue family indices
		std::vector<uint32_t> mDistinctQueueFamilies;
		// Vector of pairs of queue family indices and queue indices
		std::vector<std::tuple<uint32_t, uint32_t>> mDistinctQueues;

		// Command pools are created/stored per thread and per queue family index.
		// Queue family indices are stored within the command_pool objects, thread indices in the tuple.
		std::deque<std::tuple<std::thread::id, ak::command_pool>> mCommandPools;

		// Descriptor pools are created/stored per thread and can have a name (an integer-id). 
		// If possible, it is tried to re-use a pool. Even when re-using a pool, it might happen that
		// allocating from it might fail (because out of memory, for instance). In such cases, a new 
		// pool will be created.
		std::unordered_map<pool_id, std::vector<std::weak_ptr<ak::descriptor_pool>>> mDescriptorPools;

		ak::standard_descriptor_cache mStandardDescriptorCache;

		vk::PhysicalDeviceFeatures mRequestedPhysicalDeviceFeatures;
		vk::PhysicalDeviceVulkan12Features mRequestedVulkan12DeviceFeatures;
	};

}
