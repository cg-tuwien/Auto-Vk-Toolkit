#pragma once
#include <exekutor.hpp>

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
		
		vk::Instance vulkan_instance() const { return mInstance; }
		vk::PhysicalDevice physical_device() override { return mPhysicalDevice; }
		vk::Device device() override { return mLogicalDevice; }
		vk::DispatchLoaderDynamic dynamic_dispatch() override { return mDynamicDispatch; }

		uint32_t graphics_queue_index() const { return mGraphicsQueue.queue_index(); }
		uint32_t presentation_queue_index() const { return mPresentQueue.queue_index(); }
		uint32_t transfer_queue_index() const { return mTransferQueue.queue_index(); }

		ak::queue& graphics_queue() { return mGraphicsQueue; }
		ak::queue& presentation_queue() { return mPresentQueue; }
		ak::queue& transfer_queue() { return mTransferQueue; }
		
		const std::vector<uint32_t>& all_queue_family_indices() const { return mDistinctQueueFamilies; }

		/** Gets a command pool for the given queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 *	@param		aQueueFamilyIndex		Command buffers allocated from the resulting pool must be sent to the given queue family
		 *	@param		aFlags		Create-flags for the pool.
		 */
		ak::command_pool& get_command_pool_for(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aFlags);
		
		/** Gets a command pool for the given queue's queue family index.
		 *	If the command pool does not exist already, it will be created.
		 *	A command pool can be shared for multiple queue families if they have the same queue family index.
		 *	The pool must have exactly the flags specified, i.e. the flags specified and only the flags specified.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 *	@param		aFlags		Create-flags for the pool.
		 */
		ak::command_pool& get_command_pool_for(const ak::queue& aQueue, vk::CommandPoolCreateFlags aFlags);

		/**	Get a command pool with the "transient"-bit set, which is optimal for single-use command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		ak::command_pool& get_command_pool_for_single_use_command_buffers(const ak::queue& aQueue);
		
		/**	Get a command pool which is optimal for reusable command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		ak::command_pool& get_command_pool_for_reusable_command_buffers(const ak::queue& aQueue);
		
		/**	Get a command pool which is optimal for single-use command buffers.
		 *	@param		aQueue		Command buffers allocated from the resulting pool must be sent to the queue family
		 *							of the given queue.
		 */
		ak::command_pool& get_command_pool_for_resettable_command_buffers(const ak::queue& aQueue);
		
		/**	Creates a new window, but does not open it. Set the window's parameters
		 *	according to your requirements before opening it!
		 *
		 *  @thread_safety This function must only be called from the main thread.
		 */
		window* create_window(const std::string& _Title);

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
		static std::vector<const char*> assemble_validation_layers();

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
		 */
		void create_swap_chain_for_window(window* pWindow);

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

		vk::PhysicalDeviceFeatures mRequestedPhysicalDeviceFeatures;
		vk::PhysicalDeviceVulkan12Features mRequestedVulkan12DeviceFeatures;
	};

}
