#include <set>
#include "context_vulkan.hpp"
#include "context_generic_glfw.hpp"

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

namespace avk
{
	std::vector<const char*> context_vulkan::sRequiredInstanceExtensions = {
	};

	std::vector<const char*> context_vulkan::sRequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
		, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
		, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME
	};

	std::mutex context_vulkan::sConcurrentAccessMutex;

	std::vector<const char*> context_vulkan::assemble_validation_layers()
	{
		std::vector<const char*> supportedValidationLayers;

		bool enableValidationLayers = mSettings.mValidationLayers.mEnableInRelease;
#ifdef _DEBUG
		enableValidationLayers = true; // always true
#endif

		if (enableValidationLayers) {
			std::copy_if(
				std::begin(mSettings.mValidationLayers.mLayers), std::end(mSettings.mValidationLayers.mLayers),
				std::back_inserter(supportedValidationLayers),
				[](auto name) {
					auto supported = is_validation_layer_supported(name);
					if (!supported) {
						LOG_WARNING(std::format("Validation layer '{}' is not supported by this Vulkan instance and will not be activated.", name));
					}
					return supported;
				});
		}

		return supportedValidationLayers;
	}

	context_vulkan::~context_vulkan()
	{
		mContextState = avk::context_state::about_to_finalize;
		context().work_off_event_handlers();
		
		mLogicalDevice.waitIdle();

#if defined(AVK_USE_VMA)
		vmaDestroyAllocator(mMemoryAllocator);
#endif
		
		//// Destroy all:
		////  - swap chains,
		////  - surfaces,
		////  - and windows
		//for (auto& ptrToSwapChainData : mSurfSwap) {
		//	// Unlike images, the image views were explicitly created by us, so we need to add a similar loop to destroy them again at the end of the program [3]
		//	for (auto& imageView : ptrToSwapChainData->mSwapChainImageViews) {
		//		mLogicalDevice.destroyImageView(imageView);
		//	}
		//	mLogicalDevice.destroySwapchainKHR(ptrToSwapChainData->mSwapChain);
		//	mInstance.destroySurfaceKHR(ptrToSwapChainData->mSurface);
		//	generic_glfw::close_window(*ptrToSwapChainData->mWindow);
		//}
		//mSurfSwap.clear();

		// Clear the parent class' windows
		mWindows.clear();

		// Destroy all command pools before the queues and the device is destroyed... but AFTER the command buffers of the windows have been destroyed
		mCommandPools.clear();
		
		// Destroy logical device
		mLogicalDevice.destroy();

#ifdef _DEBUG
		// Unhook debug callback
#if LOG_LEVEL > 0
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(mInstance, mDebugUtilsCallbackHandle, nullptr); 
		}
#endif
#endif
		
		// Destroy everything
		mInstance.destroy();
	
		mContextState = avk::context_state::has_finalized;
		context().work_off_event_handlers();
	}

	void context_vulkan::check_vk_result(VkResult err)
	{
		const auto& inst = context().vulkan_instance();
#if VK_HEADER_VERSION >= 290
		vk::detail::createResultValueType(static_cast<vk::Result>(err), "check_vk_result");
#elif VK_HEADER_VERSION >= 216
		vk::createResultValueType(static_cast<vk::Result>(err), "check_vk_result");
#else
		createResultValue(static_cast<vk::Result>(err), inst, "check_vk_result");
#endif
	}

	void context_vulkan::initialize(
		settings aSettings,
		vk::PhysicalDeviceFeatures aPhysicalDeviceFeatures,
		vk::PhysicalDeviceVulkan11Features aVulkan11Features,
		vk::PhysicalDeviceVulkan12Features aVulkan12Features,
#if VK_HEADER_VERSION >= 162
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR& aAccStructureFeatures, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& aRayTracingPipelineFeatures, vk::PhysicalDeviceRayQueryFeaturesKHR& aRayQueryFeatures
#else
		vk::PhysicalDeviceRayTracingFeaturesKHR aRayTracingFeatures
#endif
#if VK_HEADER_VERSION >= 239
		, vk::PhysicalDeviceMeshShaderFeaturesEXT& aMeshShaderFeatures
#endif
	)
	{
		mSettings = std::move(aSettings);
		mRequestedPhysicalDeviceFeatures = std::move(aPhysicalDeviceFeatures);
		mRequestedVulkan11DeviceFeatures = std::move(aVulkan11Features);
		mRequestedVulkan12DeviceFeatures = std::move(aVulkan12Features);
		
		// So it begins
		create_instance();
		work_off_event_handlers();

#ifdef _DEBUG
		// Setup debug callback and enable all validation layers configured in global settings 
		setup_vk_debug_callback();

		if (std::ranges::find(mSettings.mRequiredInstanceExtensions.mExtensions, std::string(VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) != std::end(mSettings.mRequiredInstanceExtensions.mExtensions)) {
			setup_vk_debug_report_callback();
		}
#endif

		// If not already enabled...
		if (std::ranges::find(mSettings.mRequiredInstanceExtensions.mExtensions, VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME) == std::end(mSettings.mRequiredInstanceExtensions.mExtensions)) {
			// Automatically add some extensions:
			for (auto feature : mSettings.mValidationLayers.mFeaturesToEnable) {
				if (vk::ValidationFeatureEnableEXT::eGpuAssisted == feature ||
					vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot == feature ||
					vk::ValidationFeatureEnableEXT::eBestPractices == feature ||
					vk::ValidationFeatureEnableEXT::eDebugPrintf == feature ||
					vk::ValidationFeatureEnableEXT::eSynchronizationValidation == feature) {

					// Gotta enable the EXT Validation features:
					mSettings.mRequiredInstanceExtensions.add_extension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
					break;
				}
			}
		}

		// The window surface needs to be created right after the instance creation 
		// and before physical device selection, because it can actually influence 
		// the physical device selection.

		mContextState = avk::context_state::initialization_begun;
		work_off_event_handlers();

		// NOTE: Vulkan-init is not finished yet!
		// Initialization will continue after the first window (and it's surface) have been created.
		// Only after the first window's surface has been created, the vulkan context can complete
		//   initialization and enter the context state of fully_initialized.
		if ((contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) 
			|| contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_RAY_QUERY_EXTENSION_NAME))
			&& !contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) {
			mSettings.mRequiredDeviceExtensions.add_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
		}
		if (contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) 
			&& !contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) {
			mSettings.mRequiredDeviceExtensions.add_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		}

		LOG_DEBUG_VERBOSE("Picking physical device...");

		// Select the best suitable physical device which supports all requested extensions
		context().pick_physical_device();

		assert(mPhysicalDevice);
		mEnabledDeviceExtensions.assign(std::begin(sRequiredDeviceExtensions), std::end(sRequiredDeviceExtensions));
		mEnabledDeviceExtensions.insert(std::end(mEnabledDeviceExtensions), std::begin(mSettings.mRequiredDeviceExtensions.mExtensions), std::end(mSettings.mRequiredDeviceExtensions.mExtensions));
		for (auto optionalEx : mSettings.mOptionalDeviceExtensions.mExtensions) {
			if (supports_given_extensions(mPhysicalDevice, { optionalEx })) {
				mEnabledDeviceExtensions.push_back(optionalEx);
			}
		}

		work_off_event_handlers();

		LOG_DEBUG_VERBOSE("Creating logical device...");
		
		// Just get any window:
		auto* window = context().find_window([](avk::window* w) { 
			return w->handle().has_value() && static_cast<bool>(w->surface());
		});

		const vk::SurfaceKHR* surface = nullptr;
		// Do we have a window with a handle?
		if (nullptr != window) { 
			surface = &window->surface();
		}

		context().mContextState = avk::context_state::surfaces_created;
		work_off_event_handlers();

		// Do we already have a physical device?
		assert(context().physical_device());
		assert(mPhysicalDevice);

		// If the user has not created any queue, create at least one
		if (mQueues.empty()) {
			auto familyIndex = avk::queue::select_queue_family_index(mPhysicalDevice, {}, avk::queue_selection_preference::versatile_queue, nullptr != surface ? *surface : std::optional<vk::SurfaceKHR>{});
			mQueues.emplace_back(avk::queue::prepare(this, familyIndex, 0));
		}
		
		LOG_DEBUG_VERBOSE("Running vulkan create_and_assign_logical_device event handler");

		// Get the same validation layers as for the instance!
		std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
	
		// Always prepare the shading rate image features descriptor, but only use it if the extension has been requested
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV{}
			.setShadingRateImage(VK_TRUE)
			.setShadingRateCoarseSampleOrder(VK_TRUE);
		auto activateShadingRateImage = shading_rate_image_extension_requested() && supports_shading_rate_image(context().physical_device());

		auto queueCreateInfos = avk::queue::get_queue_config_for_DeviceCreateInfo(std::begin(mQueues), std::end(mQueues));
		// Iterate over all vk::DeviceQueueCreateInfo entries and set the queue priorities pointers properly (just to be safe!)
		for (auto i = 0; i < std::get<0>(queueCreateInfos).size(); ++i) {
			std::get<0>(queueCreateInfos)[i].setPQueuePriorities(std::get<1>(queueCreateInfos)[i].data());
		}

		// Enable certain device features:
		// (Build a pNext chain for further supported extensions)

		auto deviceFeatures = vk::PhysicalDeviceFeatures2()
			.setFeatures(context().mRequestedPhysicalDeviceFeatures)
			.setPNext(activateShadingRateImage ? &shadingRateImageFeatureNV : nullptr);

		auto deviceVulkan11Features = context().mRequestedVulkan11DeviceFeatures;
		deviceVulkan11Features.setPNext(&deviceFeatures);

	    auto deviceVulkan12Features = context().mRequestedVulkan12DeviceFeatures;
		deviceVulkan12Features.setPNext(&deviceVulkan11Features);

#if VK_HEADER_VERSION >= 162
		assert(nullptr == aAccStructureFeatures.pNext);
		assert(nullptr == aRayTracingPipelineFeatures.pNext);
		assert(nullptr == aRayQueryFeatures.pNext);
		if (ray_query_extension_requested()) {
			aRayQueryFeatures.setPNext(deviceFeatures.pNext);
			deviceFeatures.setPNext(&aRayQueryFeatures);
		}
		if (ray_tracing_pipeline_extension_requested()) {
			aRayTracingPipelineFeatures.setPNext(deviceFeatures.pNext);
			deviceFeatures.setPNext(&aRayTracingPipelineFeatures);
		}
		if (acceleration_structure_extension_requested()) {
			deviceVulkan12Features.setDescriptorIndexing(VK_TRUE);
			deviceVulkan12Features.setBufferDeviceAddress(VK_TRUE);

			aAccStructureFeatures.setPNext(deviceFeatures.pNext);
			deviceFeatures.setPNext(&aAccStructureFeatures);
		}
#else
		if (ray_tracing_extension_requested()) {
			aRayTracingFeatures.setPNext(deviceFeatures.pNext);
			deviceFeatures.setPNext(&aRayTracingFeatures);
			
			deviceVulkan12Features.setBufferDeviceAddress(VK_TRUE);
		}
#endif

#if VK_HEADER_VERSION >= 239
		if (is_mesh_shader_ext_requested()) {
			aMeshShaderFeatures.setPNext(deviceFeatures.pNext);
			deviceFeatures.setPNext(&aMeshShaderFeatures);
		}
#endif
		// Always prepare the mesh shader feature descriptor, but only use it if the extension has been requested
		auto meshShaderFeatureNV = VkPhysicalDeviceMeshShaderFeaturesNV{};
		meshShaderFeatureNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
		meshShaderFeatureNV.taskShader = VK_TRUE;
		meshShaderFeatureNV.meshShader = VK_TRUE;
		if (is_mesh_shader_nv_requested() && supports_mesh_shader_nv(context().physical_device())) {
			meshShaderFeatureNV.pNext = deviceFeatures.pNext;
			deviceFeatures.setPNext(&meshShaderFeatureNV);
		}

		auto dynamicRenderingFeature = VkPhysicalDeviceDynamicRenderingFeaturesKHR{};
		dynamicRenderingFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
		dynamicRenderingFeature.dynamicRendering = VK_TRUE;
		if(is_dynamic_rendering_requested() && supports_dynamic_rendering(context().physical_device())){
			dynamicRenderingFeature.pNext = deviceFeatures.pNext;
			deviceFeatures.setPNext(&dynamicRenderingFeature);
		}

		// Unconditionally enable Synchronization2, because synchronization abstraction depends on it; it is just not implemented for Synchronization1:
		auto physicalDeviceSync2Features = vk::PhysicalDeviceSynchronization2FeaturesKHR{}
			.setPNext(deviceFeatures.pNext)
			.setSynchronization2(VK_TRUE);
		deviceFeatures.setPNext(&physicalDeviceSync2Features);

		// And here add all the pNext stuff from aPhysicalDeviceFeaturePNexts
		struct DummyForVkStructs {
			VkStructureType sType;
			void* pNext;
		};
		for (auto extToBeAdded : mSettings.mPhysicalDeviceFeaturesPNextChainEntries) {
			auto* extStruct = reinterpret_cast<DummyForVkStructs*>(extToBeAdded.pNext);
			extStruct->pNext     = deviceFeatures.pNext;
			deviceFeatures.pNext = extToBeAdded.pNext;
		}

		const auto& devex = get_all_enabled_device_extensions();
		auto deviceCreateInfo = vk::DeviceCreateInfo()
			.setQueueCreateInfoCount(static_cast<uint32_t>(std::get<0>(queueCreateInfos).size()))
			.setPQueueCreateInfos(std::get<0>(queueCreateInfos).data())
			.setPNext(&deviceVulkan12Features) // instead of :setPEnabledFeatures(&deviceFeatures) because we are using vk::PhysicalDeviceFeatures2
			// Whether the device supports these extensions has already been checked during device selection in @ref pick_physical_device
			.setEnabledExtensionCount(static_cast<uint32_t>(devex.size()))
			.setPpEnabledExtensionNames(devex.data())
			.setEnabledLayerCount(static_cast<uint32_t>(supportedValidationLayers.size()))
			.setPpEnabledLayerNames(supportedValidationLayers.data());
		context().mLogicalDevice = context().physical_device().createDevice(deviceCreateInfo);

		if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_core())>, vk::DispatchLoaderDynamic&>) {
			reinterpret_cast<vk::DispatchLoaderDynamic&>(context().dispatch_loader_core()).init(context().mLogicalDevice); // stupid, because it's a constexpr, but MSVC complains otherwise
		}
		if (static_cast<void*>(&context().dispatch_loader_core()) != static_cast<void*>(&context().dispatch_loader_ext())) {
			if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_ext())>, vk::DispatchLoaderDynamic&>) {
				reinterpret_cast<vk::DispatchLoaderDynamic&>(context().dispatch_loader_ext()).init(context().mLogicalDevice); // stupid, because it's a constexpr, but MSVC complains otherwise
			}
		}

		mContextState = context_state::device_created;
		work_off_event_handlers();
		
		// Determine distinct queue family indices and distinct (family-id, queue-id)-tuples:
		std::set<uint32_t> uniqueFamilyIndices;
		std::set<std::tuple<uint32_t, uint32_t>> uniqueQueues;
		for (auto& q : mQueues) {
			uniqueFamilyIndices.insert(q.family_index());
			uniqueQueues.insert(std::make_tuple(q.family_index(), q.queue_index()));
		}
		// Put into contiguous memory
		for (auto idx : uniqueFamilyIndices) {
			context().mDistinctQueueFamilies.push_back(idx);
		}
		for (auto tpl : uniqueQueues) {
			context().mDistinctQueues.push_back(tpl);
		}

#if defined(AVK_USE_VMA)
		// With everything in place, create the memory allocator:
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physical_device();
		allocatorInfo.device = device();
		allocatorInfo.instance = vulkan_instance();
		if (std::find(std::begin(mSettings.mRequiredDeviceExtensions.mExtensions), std::end(mSettings.mRequiredDeviceExtensions.mExtensions), std::string(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) != std::end(mSettings.mRequiredDeviceExtensions.mExtensions)) {
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}
		vmaCreateAllocator(&allocatorInfo, &mMemoryAllocator);
#else
		mMemoryAllocator = std::make_tuple(physical_device(), device());
#endif
		
		context().mContextState = avk::context_state::fully_initialized;
		work_off_event_handlers();

	}

	avk::command_pool& context_vulkan::get_command_pool_for(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aFlags)
	{
		std::scoped_lock<std::mutex> guard(sConcurrentAccessMutex);
		auto it = std::find_if(std::begin(mCommandPools), std::end(mCommandPools),
							   [lThreadId = std::this_thread::get_id(), lFamilyIdx = aQueueFamilyIndex, lFlags = aFlags] (const std::tuple<std::thread::id, avk::command_pool>& existing) {
									auto& tid = std::get<0>(existing);
									auto& q = std::get<1>(existing);
									return tid == lThreadId && q->queue_family_index() == lFamilyIdx && lFlags == q->create_info().flags;
							   });
		if (it == std::end(mCommandPools)) {
			return std::get<1>(mCommandPools.emplace_back(std::this_thread::get_id(), create_command_pool(aQueueFamilyIndex, aFlags)));
		}
		return std::get<1>(*it);
	}

	avk::command_pool& context_vulkan::get_command_pool_for(const avk::queue& aQueue, vk::CommandPoolCreateFlags aFlags)
	{
		return get_command_pool_for(aQueue.family_index(), aFlags);
	}

	avk::command_pool& context_vulkan::get_command_pool_for_single_use_command_buffers(const avk::queue& aQueue)
	{
		return get_command_pool_for(aQueue, vk::CommandPoolCreateFlagBits::eTransient);
	}
	
	avk::command_pool& context_vulkan::get_command_pool_for_reusable_command_buffers(const avk::queue& aQueue)
	{
		return get_command_pool_for(aQueue, {});
	}
	
	avk::command_pool& context_vulkan::get_command_pool_for_resettable_command_buffers(const avk::queue& aQueue)
	{
		return get_command_pool_for(aQueue, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	}
	
	void context_vulkan::begin_composition()
	{ 
		dispatch_to_main_thread([]() {
			context().mContextState = avk::context_state::composition_beginning;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::end_composition()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = avk::context_state::composition_ending;
			context().work_off_event_handlers();
			
			context().mLogicalDevice.waitIdle();
		});
	}

	void context_vulkan::begin_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = avk::context_state::frame_begun;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::update_stage_done()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = avk::context_state::frame_updates_done;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::end_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = avk::context_state::frame_ended;
			context().work_off_event_handlers();
		});
	}

	avk::command_buffer context_vulkan::record_and_submit(std::vector<avk::recorded_commands_t> aRecordedCommandsAndSyncInstructions, const avk::queue& aQueue, vk::CommandBufferUsageFlags aUsageFlags)
	{
		auto& cmdPool = get_command_pool_for_single_use_command_buffers(aQueue);
		auto cmdBfr = cmdPool->alloc_command_buffer(aUsageFlags);

		record(std::move(aRecordedCommandsAndSyncInstructions))
			.into_command_buffer(cmdBfr)
			.then_submit_to(aQueue)
			.submit();

		return cmdBfr;
	}

	avk::semaphore context_vulkan::record_and_submit_with_semaphore(std::vector<avk::recorded_commands_t> aRecordedCommandsAndSyncInstructions, const avk::queue& aQueue, avk::stage::pipeline_stage_flags aSrcSignalStage, vk::CommandBufferUsageFlags aUsageFlags)
	{
		auto& cmdPool = get_command_pool_for_single_use_command_buffers(aQueue);
		auto cmdBfr = cmdPool->alloc_command_buffer(aUsageFlags);
		auto sem = create_semaphore();

		record(std::move(aRecordedCommandsAndSyncInstructions))
			.into_command_buffer(cmdBfr)
			.then_submit_to(aQueue)
			.signaling_upon_completion(aSrcSignalStage >> sem)
			.submit();

		sem->handle_lifetime_of(std::move(cmdBfr));
		return sem;
	}

	avk::fence context_vulkan::record_and_submit_with_fence(std::vector<avk::recorded_commands_t> aRecordedCommandsAndSyncInstructions, const avk::queue& aQueue, vk::CommandBufferUsageFlags aUsageFlags)
	{
		auto& cmdPool = get_command_pool_for_single_use_command_buffers(aQueue);
		auto cmdBfr = cmdPool->alloc_command_buffer(aUsageFlags);
		auto fen = create_fence();

		record(std::move(aRecordedCommandsAndSyncInstructions))
			.into_command_buffer(cmdBfr)
			.then_submit_to(aQueue)
			.signaling_upon_completion(fen)
			.submit();

		fen->handle_lifetime_of(std::move(cmdBfr));
		return fen;
	}

	avk::queue& context_vulkan::create_queue(vk::QueueFlags aRequiredFlags, avk::queue_selection_preference aQueueSelectionPreference, window* aPresentSupportForWindow, float aQueuePriority)
	{
		assert(are_we_on_the_main_thread());
		context().work_off_event_handlers();
		auto& nuQu = mQueues.emplace_back();

		auto whenToInvoke = context_state::physical_device_selected;
		if (nullptr != aPresentSupportForWindow) {
			whenToInvoke |= context_state::surfaces_created;
		}
		
		context().add_event_handler(whenToInvoke, [&nuQu, aRequiredFlags, aQueueSelectionPreference, aPresentSupportForWindow, aQueuePriority, this]() -> bool {
			LOG_DEBUG_VERBOSE("Running queue creation handler.");

			std::optional<vk::SurfaceKHR> surfaceSupport{};
			if (nullptr != aPresentSupportForWindow) {
				surfaceSupport = aPresentSupportForWindow->surface();
				if (!surfaceSupport.has_value() || !surfaceSupport.value()) {
					LOG_INFO("Have to open window which the queue depends on during initialization! Opening now...");
					aPresentSupportForWindow->open();
					return false;
				}
			}
			auto queueFamily = avk::queue::select_queue_family_index(context().physical_device(), aRequiredFlags, aQueueSelectionPreference, surfaceSupport);

			// Do we already have queues with that queue family?
			auto num = std::count_if(std::begin(mQueues), std::end(mQueues), [queueFamily](const avk::queue& q) { return q.is_prepared() && q.family_index() == queueFamily; });
#ifdef _DEBUG
			// The previous queues must be consecutively numbered. If they are not.... I have no explanation for it.
			std::vector<int> check(num, 0);
			for (int i = 0; i < num; ++i) {
				if (mQueues[i].family_index() == queueFamily) {
					check[mQueues[i].queue_index()]++;
				}
			}
			for (int i = 0; i < num; ++i) {
				assert(check[i] == 1);
			}
#endif
			
			nuQu = avk::queue::prepare(this, queueFamily, static_cast<uint32_t>(num), aQueuePriority);

			return true;
		});
		
		context().add_event_handler(context_state::device_created, [&nuQu, aPresentSupportForWindow, this]() -> bool {
			LOG_DEBUG_VERBOSE("Assigning queue handles handler.");

			nuQu.assign_handle();

			return true;
		});

		return nuQu;
	}
	
	window* context_vulkan::create_window(const std::string& aTitle)
	{
		assert(are_we_on_the_main_thread());
		context().work_off_event_handlers();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		auto* wnd = context_generic_glfw::prepare_window();
		wnd->set_title(aTitle);

		// Wait for the window to receive a valid handle before creating its surface
		context().add_event_handler(context_state::anytime_during_or_after_init | context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running window surface creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](avk::window* w) {
				return w == wnd && w->handle().has_value();
			});

			if (nullptr == window) { // not yet
				return false;
			}

			VkSurfaceKHR surface;
			if (VK_SUCCESS != glfwCreateWindowSurface(context().vulkan_instance(), wnd->handle()->mHandle, nullptr, &surface)) {
				throw avk::runtime_error(std::format("Failed to create surface for window '{}'!", wnd->title()));
			}

			vk::ObjectDestroy<vk::Instance, DISPATCH_LOADER_CORE_TYPE> deleter(context().vulkan_instance(), nullptr, context().dispatch_loader_core());
			window->mSurface = vk::UniqueHandle<vk::SurfaceKHR, DISPATCH_LOADER_CORE_TYPE>(surface, deleter);
			return true;
		});

		// Continue with swap chain creation after the context has COMPLETELY initialized
		//   and the window's handle and surface have been created
		context().add_event_handler(context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running swap chain creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](avk::window* w) { 
				return w == wnd && w->handle().has_value() && static_cast<bool>(w->surface());
			});

			if (nullptr == window) {
				return false;
			}
			// Okay, the window has a surface and vulkan has initialized. 
			// Let's create more stuff for this window!			
			wnd->create_swap_chain(window::swapchain_creation_mode::create_new_swapchain);
			return true;
		});

		return wnd;
	}

	void context_vulkan::create_instance()
	{
		if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_core())>, vk::DispatchLoaderDynamic&>) {
			reinterpret_cast<vk::DispatchLoaderDynamic&>(dispatch_loader_core()).init(vkGetInstanceProcAddr); // stupid, because it's a constexpr, but MSVC complains otherwise
		}
		if (static_cast<void*>(&context().dispatch_loader_core()) != static_cast<void*>(&context().dispatch_loader_ext())) {
			if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_ext())>, vk::DispatchLoaderDynamic&>) {
				reinterpret_cast<vk::DispatchLoaderDynamic&>(context().dispatch_loader_ext()).init(vkGetInstanceProcAddr); // stupid, because it's a constexpr, but MSVC complains otherwise
			}
		}

		// Information about the application for the instance creation call
		auto appInfo = vk::ApplicationInfo(mSettings.mApplicationName.mValue.c_str(), mSettings.mApplicationVersion.mValue,
										   "Auto-Vk-Toolkit", VK_MAKE_VERSION(0, 99, 1),
										   VK_API_VERSION_1_3);

		// GLFW requires several extensions to interface with the window system. Query them.
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> requiredExtensions;
		requiredExtensions.assign(glfwExtensions, static_cast<const char**>(glfwExtensions + glfwExtensionCount));
		requiredExtensions.insert(std::end(requiredExtensions), std::begin(sRequiredInstanceExtensions), std::end(sRequiredInstanceExtensions));
		requiredExtensions.insert(std::end(requiredExtensions), std::begin(mSettings.mRequiredInstanceExtensions.mExtensions), std::end(mSettings.mRequiredInstanceExtensions.mExtensions));

		// Check for each validation layer if it exists and activate all which do.
		std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
		// Enable extension to receive callbacks for the validation layers
		if (supportedValidationLayers.size() > 0) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		auto validationFeatures = vk::ValidationFeaturesEXT{};
		if (mSettings.mValidationLayers.mFeaturesToEnable.size() > 0) {
			validationFeatures
				.setEnabledValidationFeatureCount(static_cast<uint32_t>(mSettings.mValidationLayers.mFeaturesToEnable.size()))
				.setPEnabledValidationFeatures(mSettings.mValidationLayers.mFeaturesToEnable.data());
		}
		if (mSettings.mValidationLayers.mFeaturesToDisable.size() > 0) {
			validationFeatures
				.setDisabledValidationFeatureCount(static_cast<uint32_t>(mSettings.mValidationLayers.mFeaturesToDisable.size()))
				.setPDisabledValidationFeatures(mSettings.mValidationLayers.mFeaturesToDisable.data());
		}

		// Gather all previously prepared info for instance creation and put in one struct:
		auto instCreateInfo = vk::InstanceCreateInfo()
			.setPApplicationInfo(&appInfo)
			.setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
			.setPpEnabledExtensionNames(requiredExtensions.data())
			.setEnabledLayerCount(static_cast<uint32_t>(supportedValidationLayers.size()))
			.setPpEnabledLayerNames(supportedValidationLayers.data());

		if (validationFeatures.disabledValidationFeatureCount > 0u || validationFeatures.enabledValidationFeatureCount > 0u) {
			instCreateInfo.setPNext(&validationFeatures);
		}
		
		// Create it, errors will result in an exception.
		mInstance = vk::createInstance(instCreateInfo);
		if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_core())>, vk::DispatchLoaderDynamic&>) {
			reinterpret_cast<vk::DispatchLoaderDynamic&>(dispatch_loader_core()).init(mInstance); // stupid, because it's a constexpr, but MSVC complains otherwise
		}
		if (static_cast<void*>(&context().dispatch_loader_core()) != static_cast<void*>(&context().dispatch_loader_ext())) {
			if constexpr (std::is_same_v<std::remove_cv_t<decltype(dispatch_loader_ext())>, vk::DispatchLoaderDynamic&>) {
				reinterpret_cast<vk::DispatchLoaderDynamic&>(context().dispatch_loader_ext()).init(mInstance); // stupid, because it's a constexpr, but MSVC complains otherwise
			}
		}

	}

	bool context_vulkan::is_validation_layer_supported(const char* pName)
	{
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		return availableLayers.end() !=  std::find_if(
			std::begin(availableLayers), std::end(availableLayers), 
			[toFind = std::string(pName)](const vk::LayerProperties& e) {
				return std::string(static_cast<const char*>(e.layerName)) == toFind;
			});
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL context_vulkan::vk_debug_utils_callback(
		VkDebugUtilsMessageSeverityFlagBitsEXT pMessageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT pMessageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		// build a string from the message type parameter
		std::string typeDescription;
		if ((pMessageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) != 0) {
			typeDescription += "General, ";
		}
		if ((pMessageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) != 0) {
			typeDescription += "Validation, ";
		}
		if ((pMessageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) != 0) {
			typeDescription += "Performance, ";
		}
		// build the final string to be displayed (could also be an empty one)
		if (typeDescription.size() > 0) {
			typeDescription = "(" + typeDescription.substr(0, typeDescription.size() - 2) + ") ";
		}

		std::string messageIdName = nullptr == pCallbackData->pMessageIdName ? "(nullptr)" : std::string(pCallbackData->pMessageIdName);
		if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			assert(pCallbackData);
			LOG_ERROR___(std::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber, 
				messageIdName,
				pCallbackData->pMessage));
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			assert(pCallbackData);
			LOG_WARNING___(std::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				messageIdName,
				pCallbackData->pMessage));
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			assert(pCallbackData);
			if (std::string("Loader Message") == messageIdName) {
				LOG_VERBOSE___(std::format("Debug utils callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					messageIdName,
					pCallbackData->pMessage));
			}
			else {
				LOG_INFO___(std::format("Debug utils callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					messageIdName,
					pCallbackData->pMessage));
			}
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			assert(pCallbackData);
			LOG_VERBOSE___(std::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				messageIdName,
				pCallbackData->pMessage));
			return VK_FALSE; 
		}
		return VK_FALSE;
	}

	void context_vulkan::setup_vk_debug_callback()
	{
		assert(mInstance);
		// Configure logging
#if LOG_LEVEL > 0
		if (mSettings.mValidationLayers.mLayers.size() == 0) {
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT msgCreateInfo = {};
		msgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		msgCreateInfo.messageSeverity = 0;
		if (avk::has_flag(mSettings.mEnabledDebugUtilsMessageSeverities, vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)) {
			msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		}
#if LOG_LEVEL > 1
		if (avk::has_flag(mSettings.mEnabledDebugUtilsMessageSeverities, vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)) {
			msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
		}
#if LOG_LEVEL > 2
		if (avk::has_flag(mSettings.mEnabledDebugUtilsMessageSeverities, vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo)) {
			msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		}
#if LOG_LEVEL > 3
		if (avk::has_flag(mSettings.mEnabledDebugUtilsMessageSeverities, vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)) {
			msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		}
#endif
#endif
#endif
		msgCreateInfo.messageType = static_cast<VkDebugUtilsMessageTypeFlagsEXT>(mSettings.mEnabledDebugUtilsMessageTypes);
		msgCreateInfo.pfnUserCallback = context_vulkan::vk_debug_utils_callback;

		// Hook in
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)mInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			//VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger

			auto result = func(
				static_cast<VkInstance>(mInstance), 
				&msgCreateInfo, 
				nullptr, 
				&mDebugUtilsCallbackHandle);
			if (VK_SUCCESS != result) {
				throw avk::runtime_error("Failed to set up debug utils callback via vkCreateDebugUtilsMessengerEXT");
			}
		}
		else {
			throw avk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugUtilsMessengerEXT.");
		}
#endif
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL context_vulkan::vk_debug_report_callback(
		VkDebugReportFlagsEXT                       flags,
		VkDebugReportObjectTypeEXT                  objectType,
		uint64_t                                    object,
		size_t                                      location,
		int32_t                                     messageCode,
		const char* pLayerPrefix,
		const char* pMessage,
		void* pUserData)
	{
		if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) != 0) {
			LOG_ERROR___(std::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT(objectType)),
				pMessage));
			return VK_FALSE;
		}
		if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0) {
			LOG_WARNING___(std::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT(objectType)),
				pMessage));
			return VK_FALSE;
		}
		if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0) {
			LOG_DEBUG___(std::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT(objectType)),
				pMessage));
			return VK_FALSE;
		}
		LOG_INFO___(std::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
			to_string(vk::DebugReportFlagsEXT{ flags }),
			to_string(vk::DebugReportObjectTypeEXT(objectType)),
			pMessage));
		return VK_FALSE;
	}

	void context_vulkan::setup_vk_debug_report_callback()
	{
		assert(mInstance);
#if LOG_LEVEL > 0

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT;
#if LOG_LEVEL > 1
		createInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
#if LOG_LEVEL > 2
		createInfo.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
#endif
#endif
#if defined(_DEBUG)
		createInfo.flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
#endif
		createInfo.pfnCallback = context_vulkan::vk_debug_report_callback;
		
		// Hook in
		auto func = (PFN_vkCreateDebugReportCallbackEXT)mInstance.getProcAddr("vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			
			auto result = func(
				static_cast<VkInstance>(mInstance),
				&createInfo,
				nullptr,
				&mDebugReportCallbackHandle);
			if (VK_SUCCESS != result) {
				throw avk::runtime_error("Failed to set up debug report callback via vkCreateDebugReportCallbackEXT");
			}
		}
		else {
			throw avk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugReportCallbackEXT.");
		}

#endif
	}

	const std::vector<const char*>& context_vulkan::get_all_enabled_device_extensions() const
	{
		assert(!mEnabledDeviceExtensions.empty());
		return mEnabledDeviceExtensions;
	}

	bool context_vulkan::supports_shading_rate_image(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV{};
		supportedExtFeatures.pNext = &shadingRateImageFeatureNV;
		device.getFeatures2(&supportedExtFeatures, dispatch_loader_core());
		return shadingRateImageFeatureNV.shadingRateImage && shadingRateImageFeatureNV.shadingRateCoarseSampleOrder && supportedExtFeatures.features.shaderStorageImageExtendedFormats;
	}

	bool context_vulkan::shading_rate_image_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME)) != std::end(devex);
	}

#if VK_HEADER_VERSION >= 239
	bool context_vulkan::supports_mesh_shader_ext(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto meshShaderFeatures = vk::PhysicalDeviceMeshShaderFeaturesEXT{};
		supportedExtFeatures.pNext = &meshShaderFeatures;
		device.getFeatures2(&supportedExtFeatures, dispatch_loader_core());
		return meshShaderFeatures.meshShader == VK_TRUE && meshShaderFeatures.taskShader == VK_TRUE;
	}

	bool context_vulkan::is_mesh_shader_ext_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_EXT_MESH_SHADER_EXTENSION_NAME)) != std::end(devex);
	}
#endif

	bool context_vulkan::supports_dynamic_rendering(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceProperties2 physicalProperties;
		device.getProperties2(&physicalProperties, dispatch_loader_core());

		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeaturesKHR{};
		supportedExtFeatures.pNext = &dynamicRenderingFeatures;
		device.getFeatures2(&supportedExtFeatures, dispatch_loader_core());
		return dynamicRenderingFeatures.dynamicRendering == VK_TRUE;
	}

	bool context_vulkan::supports_mesh_shader_nv(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto meshShaderFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV{};
		supportedExtFeatures.pNext = &meshShaderFeatures;
		device.getFeatures2(&supportedExtFeatures, dispatch_loader_core());
		return meshShaderFeatures.meshShader == VK_TRUE && meshShaderFeatures.taskShader == VK_TRUE;
	}

	bool context_vulkan::is_mesh_shader_nv_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_NV_MESH_SHADER_EXTENSION_NAME)) != std::end(devex);
	}	
	bool context_vulkan::is_dynamic_rendering_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) != std::end(devex);
	}

#if VK_HEADER_VERSION >= 162
	bool context_vulkan::ray_tracing_pipeline_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) != std::end(devex);
	}

	bool context_vulkan::acceleration_structure_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) != std::end(devex);
	}

	bool context_vulkan::ray_query_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_RAY_QUERY_EXTENSION_NAME)) != std::end(devex);
	}

	bool context_vulkan::pipeline_library_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)) != std::end(devex);
	}

	bool context_vulkan::deferred_host_operations_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME)) != std::end(devex);
	}
#else
	bool context_vulkan::ray_tracing_extension_requested()
	{
		const auto& devex = get_all_enabled_device_extensions();
		return std::find(std::begin(devex), std::end(devex), std::string(VK_KHR_RAY_TRACING_EXTENSION_NAME)) != std::end(devex);
	}
#endif

	bool context_vulkan::supports_given_extensions(const vk::PhysicalDevice& aPhysicalDevice, const std::vector<const char*>& aExtensionsInQuestion) const
	{
	    // Search for each extension requested!
		auto deviceExtensions = aPhysicalDevice.enumerateDeviceExtensionProperties();
        return supports_given_extensions(deviceExtensions, aExtensionsInQuestion);
	}

	bool context_vulkan::supports_given_extensions(const std::vector<vk::ExtensionProperties>& aPhysicalDeviceExtensionsAvailable, const std::vector<const char*>& aExtensionsInQuestion) const
	{
		for (const auto& extensionName : aExtensionsInQuestion) {
			// See if we can find the current requested extension in the array of all device extensions:
            auto result = std::ranges::find_if(aPhysicalDeviceExtensionsAvailable,
                                               [extensionName](const vk::ExtensionProperties& devext) {
                                                   return strcmp(extensionName, devext.extensionName) == 0;
                                               });
            if (result == std::end(aPhysicalDeviceExtensionsAvailable)) {
				// could not find the device extension
				return false;
			}
		}
		return true; // All extensions supported

	}

	void context_vulkan::pick_physical_device()
	{
		assert(mInstance);
		auto physicalDevices = mInstance.enumeratePhysicalDevices();
		if (physicalDevices.size() == 0) {
			throw avk::runtime_error("Failed to find GPUs with Vulkan support.");
		}
		const vk::PhysicalDevice* currentSelection = nullptr;
		uint32_t currentScore = 0; // device score
		
		// Iterate over all devices
		for (const auto& physicalDevice : physicalDevices) {
			// get features and queues
			auto properties = physicalDevice.getProperties();
			const auto supportedFeatures = physicalDevice.getFeatures();
			auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
			// check for required features
			bool graphicsBitSet = false;
			bool computeBitSet = false;
			for (const auto& qfp : queueFamilyProps) {
				graphicsBitSet = graphicsBitSet || ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics);
				computeBitSet = computeBitSet || ((qfp.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute);
			}

			uint32_t score =
				(graphicsBitSet ? 10'000 : 0) +
				(computeBitSet  ? 10'000 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 10 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu ? 5 : 0);

			const auto deviceName = std::string(static_cast<const char*>(properties.deviceName));
			
			if (!mSettings.mPhysicalDeviceSelectionHint.mValue.empty()) {
				score += avk::find_case_insensitive(deviceName, mSettings.mPhysicalDeviceSelectionHint.mValue, 0) != std::string::npos ? 100'000 : 0;
			}

			// Check if anisotropy is supported
			if (supportedFeatures.samplerAnisotropy) {
				score += 30;
			}
			else {
				LOG_INFO(std::format("Physical device \"{}\" does not support samplerAnisotropy.", properties.deviceName.data()));
			}

			// Check if descriptor indexing is supported
			{
				auto indexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeaturesEXT{};
				auto features = vk::PhysicalDeviceFeatures2{}
					.setPNext(&indexingFeatures);
				physicalDevice.getFeatures2(&features, dispatch_loader_core());
				if (indexingFeatures.descriptorBindingVariableDescriptorCount > 0) {
				    score += 40;
				}
				else {
					LOG_INFO(std::format("Physical device \"{}\" does not provide any descriptorBindingVariableDescriptor.", properties.deviceName.data()));
				}
			}

			// Check if Auto-Vk-Toolkit-required extensions are supported
            auto deviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
            if (!supports_given_extensions(deviceExtensions, sRequiredDeviceExtensions)) {
				LOG_WARNING(std::format("Depreciating physical device \"{}\" because it does not support all extensions required by Auto-Vk-Toolkit.", properties.deviceName.data()));
                for (const auto& extensionName : sRequiredDeviceExtensions) {
                    auto extensionInfo = std::string("    - ") + extensionName + " ...";
                    while (extensionInfo.length() < 60) {
                        extensionInfo += ".";
                    }
                    auto result = std::ranges::find_if(deviceExtensions, [extensionName](const vk::ExtensionProperties& devext) { return strcmp(extensionName, devext.extensionName.data()) == 0; });
                    extensionInfo += result != deviceExtensions.end() ? " supported" : " NOT supported";
                    LOG_WARNING(extensionInfo);
                }
				score = 0;
			}

			// Check if user-requested extensions are supported
			if (!supports_given_extensions(physicalDevice, mSettings.mRequiredDeviceExtensions.mExtensions)) {
				LOG_WARNING(std::format("Depreciating physical device \"{}\" because it does not support all extensions required by the application.", properties.deviceName.data()));
                for (const auto& extensionName : mSettings.mRequiredDeviceExtensions.mExtensions) {
                    auto extensionInfo = std::string("    - ") + extensionName + " ...";
                    while (extensionInfo.length() < 60) {
                        extensionInfo += ".";
                    }
                    auto result           = std::ranges::find_if(deviceExtensions, [extensionName](const vk::ExtensionProperties& devext) { return strcmp(extensionName, devext.extensionName.data()) == 0; });
                    extensionInfo += result != deviceExtensions.end() ? " supported" : " NOT supported";
                    LOG_WARNING(extensionInfo);
                }
				score = 0;
			}

			// Check which optional extensions are supported
			for (auto ex : mSettings.mOptionalDeviceExtensions.mExtensions) {
				if (supports_given_extensions(physicalDevice, { ex })) {
				    score += 100;
			    }
				else {
					LOG_WARNING(std::format("Physical device \"{}\" does not support the optional extension \"{}\".", properties.deviceName.data(), ex));
				}
			}

			if (score > currentScore) {
				currentSelection = &physicalDevice;
				currentScore = score;
			}	
		}

		// Handle failure:
		if (nullptr == currentSelection) {
			if (mSettings.mRequiredDeviceExtensions.mExtensions.size() > 0) {
				throw avk::runtime_error("Could not find a suitable physical device, most likely because no device supported all required device extensions.");
			}
			throw avk::runtime_error("Could not find a suitable physical device.");
		}

		// Handle success:
		mPhysicalDevice = *currentSelection;
		mContextState = avk::context_state::physical_device_selected;
		LOG_INFO(std::format("Going to use {}", mPhysicalDevice.getProperties().deviceName.data()));
	}

	glm::uvec2 context_vulkan::get_resolution_for_window(window* aWindow)
	{
		auto srfCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(aWindow->surface());

		// Vulkan tells us to match the resolution of the window by setting the width and height in the 
		// currentExtent member. However, some window managers do allow us to differ here and this is 
		// indicated by setting the width and height in currentExtent to a special value: the maximum 
		// value of uint32_t. In that case we'll pick the resolution that best matches the window within 
		// the minImageExtent and maxImageExtent bounds. [2]
		auto extent = srfCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()
			? glm::clamp(aWindow->resolution(),
						 glm::uvec2(srfCaps.minImageExtent.width, srfCaps.minImageExtent.height),
						 glm::uvec2(srfCaps.maxImageExtent.width, srfCaps.maxImageExtent.height))
			: glm::uvec2(srfCaps.currentExtent.width, srfCaps.currentExtent.height);

		return extent;
	}

	// function get current context time from outside 
	// without knowledge about the entire context content
	double get_context_time() {		
		return context().get_time();
	}

	bool are_we_on_the_main_thread() {
		return context().are_we_on_the_main_thread();
	}
}
