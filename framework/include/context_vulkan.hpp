#pragma once
#include <gvk.hpp>

namespace gvk
{	
	// ============================== VULKAN CONTEXT ================================
	/**	@brief Context for Vulkan
	 *
	 *	This context abstracts calls to the Vulkan API, for environment-related
	 *	stuff, like window creation etc.,  it relies on GLFW and inherits
	 *	@ref generic_glfw.
	 */
	class context_vulkan : public context_generic_glfw, public avk::root
	{
	public:
		context_vulkan() = default;
		context_vulkan(const context_vulkan&) = delete;
		context_vulkan(context_vulkan&&) = delete;
		context_vulkan& operator=(const context_vulkan&) = delete;
		context_vulkan& operator=(context_vulkan&&) = delete;
		~context_vulkan();

		// Checks a VkResult return type and handles it according to the current Vulkan-Hpp config
		static void check_vk_result(VkResult err);

		void initialize(
			settings aSettings,
			vk::PhysicalDeviceFeatures aPhysicalDeviceFeatures,
			vk::PhysicalDeviceVulkan12Features aVulkan12Features,
#if VK_HEADER_VERSION >= 162
			vk::PhysicalDeviceAccelerationStructureFeaturesKHR& aAccStructureFeatures, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& aRayTracingPipelineFeatures, vk::PhysicalDeviceRayQueryFeaturesKHR& aRayQueryFeatures
#else
			vk::PhysicalDeviceRayTracingFeaturesKHR aRayTracingFeatures
#endif
		);
		
		vk::Instance& vulkan_instance() { return mInstance; }
		vk::PhysicalDevice& physical_device() override { return mPhysicalDevice; }
		vk::Device& device() override { return mLogicalDevice; }
		vk::DispatchLoaderDynamic& dynamic_dispatch() override { return mDynamicDispatch; }
#if defined(AVK_USE_VMA)
		VmaAllocator& memory_allocator() override { return mMemoryAllocator; }
#else
		std::tuple<vk::PhysicalDevice, vk::Device>& memory_allocator() override { return mMemoryAllocator; }
#endif
		
		const std::vector<uint32_t>& all_queue_family_indices() const { return mDistinctQueueFamilies; }

		/** Gets a command pool for the given queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 *	@param		aQueueFamilyIndex		Command buffers allocated from the resulting pool must be sent to the given queue family
		 *	@param		aFlags		Create-flags for the pool.
		 */
		avk::command_pool& get_command_pool_for(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aFlags);
		
		/** Gets a command pool for the given queue's queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	A command pool can be shared for multiple queue families if they have the same queue family index.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 *	@param		aFlags		Create-flags for the pool.
		 */
		avk::command_pool& get_command_pool_for(const avk::queue& aQueue, vk::CommandPoolCreateFlags aFlags);

		/**	Get a command pool with the "transient"-bit set, which is optimal for single-use command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		avk::command_pool& get_command_pool_for_single_use_command_buffers(const avk::queue& aQueue);
		
		/**	Get a command pool which is optimal for reusable command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		avk::command_pool& get_command_pool_for_reusable_command_buffers(const avk::queue& aQueue);
		
		/**	Get a command pool which is optimal for single-use command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		avk::command_pool& get_command_pool_for_resettable_command_buffers(const avk::queue& aQueue);

		avk::queue& create_queue(vk::QueueFlags aRequiredFlags = {}, avk::queue_selection_preference aQueueSelectionPreference = avk::queue_selection_preference::versatile_queue, window* aPresentSupportForWindow = nullptr, float aQueuePriority = 0.5f);
		
		/**	Creates a new window, but does not open it. Set the window's parameters
		 *	according to your requirements before opening it!
		 *
		 *  @thread_safety This function must only be called from the main thread.
		 */
		window* create_window(const std::string& aTitle);

		/** Used to signal the context about the beginning of a composition */
		void begin_composition();

		/** Used to signal the context about the end of a composition */
		void end_composition();

		/** Used to signal the context about the beginning of a new frame */
		void begin_frame();

		/** Used to signal the context about the point within a frame
		 *	when all updates have been performed.
		 *	This usually means that the main thread is about to be awoken
		 *	(but has not been yet) in order to start handling input and stuff.
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
		std::vector<const char*> assemble_validation_layers();

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
		std::vector<const char*> get_all_required_device_extensions();

		/** Checks if the given physical device supports the shading rate image feature
		 */
		bool supports_shading_rate_image(const vk::PhysicalDevice& device);
		bool supports_mesh_shader(const vk::PhysicalDevice& device);

		bool shading_rate_image_extension_requested();
		bool mesh_shader_extension_requested();
#if VK_HEADER_VERSION >= 162
		bool ray_tracing_pipeline_extension_requested();
		bool acceleration_structure_extension_requested();
		bool ray_query_extension_requested();
		bool pipeline_library_extension_requested();
		bool deferred_host_operations_extension_requested();
#else
		bool ray_tracing_extension_requested();
#endif
		
		/** Checks whether the given physical device supports all the required extensions,
		 *	namely those stored in @ref settings::gRequiredDeviceExtensions. 
		 *	Returns true if it does, false otherwise.
		 */
		bool supports_all_required_extensions(const vk::PhysicalDevice& device);

		/** Pick the physical device which looks to be the most promising one */
		void pick_physical_device();

		/** Gets the right resolution for the given window, considering the window's size and surface capabilities */
		glm::uvec2 get_resolution_for_window(window* aWindow);

		//pipeline create_ray_tracing_pipeline(
		//	const std::vector<std::tuple<shader_type, shader*>>& pShaderInfos,
		//	const std::vector<vk::DescriptorSetLayout>& pDescriptorSets);

		//std::vector<framebuffer> create_framebuffers(const vk::RenderPass& renderPass, window* pWindow, const image_view_t& pDepthImageView);

		auto requested_physical_device_features() const { return mRequestedPhysicalDeviceFeatures; }
		void set_requested_physical_device_features(vk::PhysicalDeviceFeatures aNewValue) { mRequestedPhysicalDeviceFeatures = aNewValue; }
		auto requested_vulkan12_device_features() const { return mRequestedVulkan12DeviceFeatures; }
		void set_requested_vulkan12_device_features(vk::PhysicalDeviceVulkan12Features aNewValue) { mRequestedVulkan12DeviceFeatures = aNewValue; }
		
	public:
		static std::vector<const char*> sRequiredDeviceExtensions;

		// A mutex which protects the vulkan context from concurrent access from different threads
		// e.g. during parallel recording of command buffers.
		// It is only used in some functions, which are expected to be called during runtime (i.e.
		// during `invokee::update` or invokee::render` calls), not during initialization.
		static std::mutex sConcurrentAccessMutex;

		settings mSettings;
		
		vk::Instance mInstance;
		VkDebugUtilsMessengerEXT mDebugUtilsCallbackHandle;
		VkDebugReportCallbackEXT mDebugReportCallbackHandle;
		//std::vector<swap_chain_data_ptr> mSurfSwap;
		vk::PhysicalDevice mPhysicalDevice;
		vk::Device mLogicalDevice;
		vk::DispatchLoaderDynamic mDynamicDispatch;
		
#if defined(AVK_USE_VMA)
		VmaAllocator mMemoryAllocator;
#else
		std::tuple<vk::PhysicalDevice, vk::Device> mMemoryAllocator;
#endif

		// Vector of queue family indices
		std::vector<uint32_t> mDistinctQueueFamilies;
		// Vector of pairs of queue family indices and queue indices
		std::vector<std::tuple<uint32_t, uint32_t>> mDistinctQueues;

		// Command pools are created/stored per thread and per queue family index.
		// Queue family indices are stored within the command_pool objects, thread indices in the tuple.
		std::deque<std::tuple<std::thread::id, avk::command_pool>> mCommandPools;

		vk::PhysicalDeviceFeatures mRequestedPhysicalDeviceFeatures;
		vk::PhysicalDeviceVulkan12Features mRequestedVulkan12DeviceFeatures;

		std::deque<avk::queue> mQueues;
	};

}
