#include <gvk.hpp>

#include <set>

namespace gvk
{
	std::vector<const char*> context_vulkan::sRequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
						LOG_WARNING(fmt::format("Validation layer '{}' is not supported by this Vulkan instance and will not be activated.", name));
					}
					return supported;
				});
		}

		return supportedValidationLayers;
	}

	context_vulkan::~context_vulkan()
	{
		mContextState = gvk::context_state::about_to_finalize;
		context().work_off_event_handlers();

		avk::sync::sPoolToAllocCommandBuffersFrom = avk::command_pool{};
		avk::sync::sQueueToUse = nullptr;

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
	
		mContextState = gvk::context_state::has_finalized;
		context().work_off_event_handlers();
	}

	void context_vulkan::check_vk_result(VkResult err)
	{
		const auto& inst = context().vulkan_instance();
		createResultValue(static_cast<vk::Result>(err), inst, "check_vk_result");
	}

	void context_vulkan::initialize(
		settings aSettings,
		vk::PhysicalDeviceFeatures aPhysicalDeviceFeatures,
		vk::PhysicalDeviceVulkan12Features aVulkan12Features,
		vk::PhysicalDeviceRayTracingFeaturesKHR aRayTracingFeatures
	)
	{
		mSettings = std::move(aSettings);
		mRequestedPhysicalDeviceFeatures = std::move(aPhysicalDeviceFeatures);
		mRequestedVulkan12DeviceFeatures = std::move(aVulkan12Features);
		
		// So it begins
		create_instance();
		work_off_event_handlers();

#ifdef _DEBUG
		// Setup debug callback and enable all validation layers configured in global settings 
		setup_vk_debug_callback();

		if (std::find(std::begin(mSettings.mRequiredInstanceExtensions.mExtensions), std::end(mSettings.mRequiredInstanceExtensions.mExtensions), VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != std::end(mSettings.mRequiredInstanceExtensions.mExtensions)) {
			setup_vk_debug_report_callback();
		}
#endif

		// The window surface needs to be created right after the instance creation 
		// and before physical device selection, because it can actually influence 
		// the physical device selection.

		mContextState = gvk::context_state::initialization_begun;
		work_off_event_handlers();

		// NOTE: Vulkan-init is not finished yet!
		// Initialization will continue after the first window (and it's surface) have been created.
		// Only after the first window's surface has been created, the vulkan context can complete
		//   initialization and enter the context state of fully_initialized.

		LOG_DEBUG_VERBOSE("Picking physical device...");

		// Select the best suitable physical device which supports all requested extensions
		context().pick_physical_device();
		work_off_event_handlers();

		LOG_DEBUG_VERBOSE("Creating logical device...");
		
		// Just get any window:
		auto* window = context().find_window([](gvk::window* w) { 
			return w->handle().has_value() && static_cast<bool>(w->surface());
		});

		const vk::SurfaceKHR* surface = nullptr;
		// Do we have a window with a handle?
		if (nullptr != window) { 
			surface = &window->surface();
		}

		context().mContextState = gvk::context_state::surfaces_created;
		work_off_event_handlers();

		// Do we already have a physical device?
		assert(context().physical_device());
		assert(mPhysicalDevice);

		// If the user has not created any queue, create at least one
		if (mQueues.empty()) {
			auto familyIndex = avk::queue::select_queue_family_index(mPhysicalDevice, {}, avk::queue_selection_preference::versatile_queue, nullptr != surface ? *surface : std::optional<vk::SurfaceKHR>{});
			mQueues.emplace_back(avk::queue::prepare(mPhysicalDevice, familyIndex, 0));
		}
		
		LOG_DEBUG_VERBOSE("Running vulkan create_and_assign_logical_device event handler");

		// Get the same validation layers as for the instance!
		std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
	
		// Always prepare the shading rate image features descriptor, but only use it if the extension has been requested
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV{}
			.setShadingRateImage(VK_TRUE)
			.setShadingRateCoarseSampleOrder(VK_TRUE);
		auto activateShadingRateImage = shading_rate_image_extension_requested() && supports_shading_rate_image(context().physical_device());

		// Always prepare the mesh shader feature descriptor, but only use it if the extension has been requested
		auto meshShaderFeatureNV = VkPhysicalDeviceMeshShaderFeaturesNV{};
		meshShaderFeatureNV.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
		meshShaderFeatureNV.taskShader = VK_TRUE;
		meshShaderFeatureNV.meshShader = VK_TRUE;
		auto activateMeshShaderFeature = mesh_shader_extension_requested() && supports_mesh_shader(context().physical_device());

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

	    auto deviceVulkan12Features = context().mRequestedVulkan12DeviceFeatures;
		deviceVulkan12Features.setPNext(&deviceFeatures);

		if (ray_tracing_extension_requested()) {
			aRayTracingFeatures.setPNext(&deviceFeatures);
			deviceVulkan12Features.setPNext(&aRayTracingFeatures);
			deviceVulkan12Features.setBufferDeviceAddress(VK_TRUE);
		}

		if (activateMeshShaderFeature) {
			meshShaderFeatureNV.pNext = deviceVulkan12Features.pNext;
			deviceVulkan12Features.setPNext(&meshShaderFeatureNV);
		}
		
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		auto deviceCreateInfo = vk::DeviceCreateInfo()
			.setQueueCreateInfoCount(static_cast<uint32_t>(std::get<0>(queueCreateInfos).size()))
			.setPQueueCreateInfos(std::get<0>(queueCreateInfos).data())
			.setPNext(&deviceVulkan12Features) // instead of :setPEnabledFeatures(&deviceFeatures) because we are using vk::PhysicalDeviceFeatures2
			// Whether the device supports these extensions has already been checked during device selection in @ref pick_physical_device
			// TODO: Are these the correct extensions to set here?
			.setEnabledExtensionCount(static_cast<uint32_t>(allRequiredDeviceExtensions.size()))
			.setPpEnabledExtensionNames(allRequiredDeviceExtensions.data())
			.setEnabledLayerCount(static_cast<uint32_t>(supportedValidationLayers.size()))
			.setPpEnabledLayerNames(supportedValidationLayers.data());
		context().mLogicalDevice = context().physical_device().createDevice(deviceCreateInfo);
		// Create a dynamic dispatch loader for extensions
		context().mDynamicDispatch = vk::DispatchLoaderDynamic{
			context().vulkan_instance(), 
			vkGetInstanceProcAddr, // TODO: <-- Is this the right choice? There's also glfwGetInstanceProcAddress.. just saying.
			context().device()
		};

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

		// TODO: Remove sync
		avk::sync::sPoolToAllocCommandBuffersFrom = gvk::context().create_command_pool(mQueues.front().family_index(), {});
		avk::sync::sQueueToUse = &mQueues.front();

#if defined(AVK_USE_VMA)
		// With everything in place, create the memory allocator:
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physical_device();
		allocatorInfo.device = device();
		allocatorInfo.instance = vulkan_instance();
		if (avk::contains(mSettings.mRequiredDeviceExtensions.mExtensions, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}
		vmaCreateAllocator(&allocatorInfo, &mMemoryAllocator);
#else
		mMemoryAllocator = std::make_tuple(physical_device(), device());
#endif
		
		context().mContextState = gvk::context_state::fully_initialized;
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
			context().mContextState = gvk::context_state::composition_beginning;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::end_composition()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = gvk::context_state::composition_ending;
			context().work_off_event_handlers();
			
			context().mLogicalDevice.waitIdle();
		});
	}

	void context_vulkan::begin_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = gvk::context_state::frame_begun;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::update_stage_done()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = gvk::context_state::frame_updates_done;
			context().work_off_event_handlers();
		});
	}

	void context_vulkan::end_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = gvk::context_state::frame_ended;
			context().work_off_event_handlers();
		});
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
			
			nuQu = avk::queue::prepare(context().physical_device(), queueFamily, static_cast<uint32_t>(num), aQueuePriority);

			return true;
		});
		
		context().add_event_handler(context_state::device_created, [&nuQu, aPresentSupportForWindow, this]() -> bool {
			LOG_DEBUG_VERBOSE("Assigning queue handles handler.");

			nuQu.assign_handle(device());

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
			auto* window = context().find_window([wnd](gvk::window* w) {
				return w == wnd && w->handle().has_value();
			});

			if (nullptr == window) { // not yet
				return false;
			}

			VkSurfaceKHR surface;
			if (VK_SUCCESS != glfwCreateWindowSurface(context().vulkan_instance(), wnd->handle()->mHandle, nullptr, &surface)) {
				throw gvk::runtime_error(fmt::format("Failed to create surface for window '{}'!", wnd->title()));
			}

			vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic> deleter(context().vulkan_instance(), nullptr, vk::DispatchLoaderStatic());
			window->mSurface = vk::UniqueHandle<vk::SurfaceKHR, vk::DispatchLoaderStatic>(surface, deleter);
			return true;
		});

		// Continue with swap chain creation after the context has COMPLETELY initialized
		//   and the window's handle and surface have been created
		context().add_event_handler(context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running swap chain creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](gvk::window* w) { 
				return w == wnd && w->handle().has_value() && static_cast<bool>(w->surface());
			});

			if (nullptr == window) {
				return false;
			}
			// Okay, the window has a surface and vulkan has initialized. 
			// Let's create more stuff for this window!
			context().create_swap_chain_for_window(wnd);
			return true;
		});

		return wnd;
	}

	void context_vulkan::create_instance()
	{
		// Information about the application for the instance creation call
		auto appInfo = vk::ApplicationInfo(mSettings.mApplicationName.mValue.c_str(), mSettings.mApplicationVersion.mValue,
										   "Gears-Vk", VK_MAKE_VERSION(0, 1, 0), // TODO: Real version of Gears-Vk
										   VK_API_VERSION_1_2);

		// GLFW requires several extensions to interface with the window system. Query them.
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> requiredExtensions;
		requiredExtensions.assign(glfwExtensions, static_cast<const char**>(glfwExtensions + glfwExtensionCount));
		requiredExtensions.insert(
			std::end(requiredExtensions),
			std::begin(mSettings.mRequiredInstanceExtensions.mExtensions), std::end(mSettings.mRequiredInstanceExtensions.mExtensions));

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
		mDynamicDispatch.init((VkInstance)mInstance, vkGetInstanceProcAddr);
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

		if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			assert(pCallbackData);
			LOG_ERROR__(fmt::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber, 
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			assert(pCallbackData);
			LOG_WARNING__(fmt::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			assert(pCallbackData);
			if (std::string("Loader Message") == pCallbackData->pMessageIdName) {
				LOG_VERBOSE__(fmt::format("Debug utils callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					pCallbackData->pMessageIdName,
					pCallbackData->pMessage));
			}
			else {
				LOG_INFO__(fmt::format("Debug utils callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					pCallbackData->pMessageIdName,
					pCallbackData->pMessage));
			}
			return VK_FALSE;
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			assert(pCallbackData);
			LOG_VERBOSE__(fmt::format("Debug utils callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
			return VK_FALSE; 
		}
		return VK_TRUE;
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
		msgCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
#if LOG_LEVEL > 1
		msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
#if LOG_LEVEL > 2
		msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
#if LOG_LEVEL > 3
		msgCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
#endif
#endif
#endif
		msgCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
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
				throw gvk::runtime_error("Failed to set up debug utils callback via vkCreateDebugUtilsMessengerEXT");
			}
		}
		else {
			throw gvk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugUtilsMessengerEXT.");
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
			LOG_ERROR__(fmt::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT{objectType}),
				pMessage));
			return VK_FALSE;
		}
		if ((flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) != 0) {
			LOG_WARNING__(fmt::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT{ objectType }),
				pMessage));
			return VK_FALSE;
		}
		if ((flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) != 0) {
			LOG_DEBUG__(fmt::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
				to_string(vk::DebugReportFlagsEXT{ flags }),
				to_string(vk::DebugReportObjectTypeEXT{ objectType }),
				pMessage));
			return VK_FALSE;
		}
		LOG_INFO__(fmt::format("Debug Report callback with flags[{}], object-type[{}], and Message[{}]",
			to_string(vk::DebugReportFlagsEXT{ flags }),
			to_string(vk::DebugReportObjectTypeEXT{ objectType }),
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
				throw gvk::runtime_error("Failed to set up debug report callback via vkCreateDebugReportCallbackEXT");
			}
		}
		else {
			throw gvk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugReportCallbackEXT.");
		}

#endif
	}

	std::vector<const char*> context_vulkan::get_all_required_device_extensions()
	{
		std::vector<const char*> combined;
		combined.assign(std::begin(mSettings.mRequiredDeviceExtensions.mExtensions), std::end(mSettings.mRequiredDeviceExtensions.mExtensions));
		combined.insert(std::end(combined), std::begin(sRequiredDeviceExtensions), std::end(sRequiredDeviceExtensions));
		return combined;
	}

	bool context_vulkan::supports_shading_rate_image(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV{};
		supportedExtFeatures.pNext = &shadingRateImageFeatureNV;
		device.getFeatures2(&supportedExtFeatures);
		return shadingRateImageFeatureNV.shadingRateImage && shadingRateImageFeatureNV.shadingRateCoarseSampleOrder && supportedExtFeatures.features.shaderStorageImageExtendedFormats;
	}

	bool context_vulkan::supports_mesh_shader(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto meshShaderFeaturesNV = vk::PhysicalDeviceMeshShaderFeaturesNV{};
		supportedExtFeatures.pNext = &meshShaderFeaturesNV;
		device.getFeatures2(&supportedExtFeatures);
		return meshShaderFeaturesNV.meshShader == VK_TRUE && meshShaderFeaturesNV.taskShader == VK_TRUE;
	}

	bool context_vulkan::shading_rate_image_extension_requested()
	{
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		return std::find(std::begin(allRequiredDeviceExtensions), std::end(allRequiredDeviceExtensions), VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME) != std::end(allRequiredDeviceExtensions);
	}

	bool context_vulkan::mesh_shader_extension_requested()
	{
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		return std::find(std::begin(allRequiredDeviceExtensions), std::end(allRequiredDeviceExtensions), VK_NV_MESH_SHADER_EXTENSION_NAME) != std::end(allRequiredDeviceExtensions);
	}	

	bool context_vulkan::ray_tracing_extension_requested()
	{
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		return std::find(std::begin(allRequiredDeviceExtensions), std::end(allRequiredDeviceExtensions), VK_KHR_RAY_TRACING_EXTENSION_NAME) != std::end(allRequiredDeviceExtensions);
	}

	bool context_vulkan::supports_all_required_extensions(const vk::PhysicalDevice& device)
	{
		bool allExtensionsSupported = true;
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		if (allRequiredDeviceExtensions.size() > 0) {
			// Search for each extension requested!
			for (const auto& required : allRequiredDeviceExtensions) {
				auto deviceExtensions = device.enumerateDeviceExtensionProperties();
				// See if we can find the current requested extension in the array of all device extensions
				auto result = std::find_if(std::begin(deviceExtensions), std::end(deviceExtensions),
										   [required](const vk::ExtensionProperties& devext) {
											   return strcmp(required, devext.extensionName) == 0;
										   });
				if (result == std::end(deviceExtensions)) {
					// could not find the device extension
					allExtensionsSupported = false;
				}
			}
		}

		auto shadingRateImageExtensionRequired = shading_rate_image_extension_requested();
		allExtensionsSupported = allExtensionsSupported && (!shadingRateImageExtensionRequired || shadingRateImageExtensionRequired && supports_shading_rate_image(device));

		return allExtensionsSupported;
	}

	void context_vulkan::pick_physical_device()
	{
		assert(mInstance);
		auto devices = mInstance.enumeratePhysicalDevices();
		if (devices.size() == 0) {
			throw gvk::runtime_error("Failed to find GPUs with Vulkan support.");
		}
		const vk::PhysicalDevice* currentSelection = nullptr;
		uint32_t currentScore = 0; // device score
		
		// Iterate over all devices
		for (const auto& device : devices) {
			// get features and queues
			auto properties = device.getProperties();
			const auto supportedFeatures = device.getFeatures();
			auto queueFamilyProps = device.getQueueFamilyProperties();
			// check for required features
			bool graphicsBitSet = false;
			bool computeBitSet = false;
			for (const auto& qfp : queueFamilyProps) {
				graphicsBitSet = graphicsBitSet || ((qfp.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics);
				computeBitSet = computeBitSet || ((qfp.queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute);
			}

			uint32_t score =
				(graphicsBitSet ? 10 : 0) +
				(computeBitSet ? 10 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 10 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu ? 5 : 0);

			const auto deviceName = std::string(static_cast<const char*>(properties.deviceName));
			
			if (!mSettings.mPhysicalDeviceSelectionHint.mValue.empty()) {
				score += avk::find_case_insensitive(deviceName, mSettings.mPhysicalDeviceSelectionHint.mValue, 0) != std::string::npos ? 1000 : 0;
			}

			// Check if extensions are required
			if (!supports_all_required_extensions(device)) {
				LOG_WARNING(fmt::format("Depreciating physical device \"{}\" because it does not support all required extensions.", properties.deviceName));
				score = 0;
			}

			// Check if anisotropy is supported
			if (!supportedFeatures.samplerAnisotropy) {
				LOG_WARNING(fmt::format("Depreciating physical device \"{}\" because it does not sampler anisotropy.", properties.deviceName));
				score = 0;
			}

			// Check if descriptor indexing is supported
			{
				//auto indexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeaturesEXT{};
				//auto features = vk::PhysicalDeviceFeatures2KHR{}
				//	.setPNext(&indexingFeatures);
				//device.getFeatures2KHR(&features, dynamic_dispatch());

				//if (!indexingFeatures.descriptorBindingVariableDescriptorCount) {
				//	score = 0;
				//}
			}

			if (score > currentScore) {
				currentSelection = &device;
				currentScore = score;
			}	
		}

		// Handle failure:
		if (nullptr == currentSelection) {
			if (mSettings.mRequiredDeviceExtensions.mExtensions.size() > 0) {
				throw gvk::runtime_error("Could not find a suitable physical device, most likely because no device supported all required device extensions.");
			}
			throw gvk::runtime_error("Could not find a suitable physical device.");
		}

		// Handle success:
		mPhysicalDevice = *currentSelection;
		mContextState = gvk::context_state::physical_device_selected;
		LOG_INFO(fmt::format("Going to use {}", mPhysicalDevice.getProperties().deviceName));
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

	void context_vulkan::create_swap_chain_for_window(window* aWindow)
	{
		auto srfCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(aWindow->surface());
		auto extent = get_resolution_for_window(aWindow);
		auto surfaceFormat = aWindow->get_config_surface_format(aWindow->surface());

		aWindow->mImageUsage = avk::image_usage::color_attachment			 | avk::image_usage::transfer_destination		| avk::image_usage::presentable;
		const vk::ImageUsageFlags swapChainImageUsageVk =	vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
		aWindow->mImageCreateInfoSwapChain = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFormat(surfaceFormat.format)
			.setExtent(vk::Extent3D(extent.x, extent.y))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(swapChainImageUsageVk)
			.setInitialLayout(vk::ImageLayout::eUndefined);

		// Handle queue family ownership:

		std::vector<uint32_t> queueFamilyIndices;
		for (auto& getter : aWindow->mQueueFamilyIndicesGetter) {
			auto familyIndex = getter();
			if (std::end(queueFamilyIndices) == std::find(std::begin(queueFamilyIndices), std::end(queueFamilyIndices), familyIndex)) {
				queueFamilyIndices.push_back(familyIndex);
			}
		}
		
		switch (queueFamilyIndices.size()) {
		case 0:
			throw gvk::runtime_error(fmt::format("You must assign at least set one queue(family) to window '{}'! You can use add_queue_family_ownership().", aWindow->title()));
		case 1:
			aWindow->mImageCreateInfoSwapChain
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0u)
				.setPQueueFamilyIndices(&queueFamilyIndices[0]); // could also leave at nullptr!
			break;
		default:
			// Have to use separate queue families!
			// If the queue families differ, then we'll be using the concurrent mode [2]
			aWindow->mImageCreateInfoSwapChain
				.setSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(static_cast<uint32_t>(queueFamilyIndices.size()))
				.setPQueueFamilyIndices(queueFamilyIndices.data());
			break;
		}

		// With all settings gathered, create the swap chain!
		aWindow->mSwapChainCreateInfo = vk::SwapchainCreateInfoKHR{}
			.setSurface(aWindow->surface())
			.setMinImageCount(aWindow->get_config_number_of_presentable_images())
			.setImageFormat(aWindow->mImageCreateInfoSwapChain.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(vk::Extent2D{ aWindow->mImageCreateInfoSwapChain.extent.width, aWindow->mImageCreateInfoSwapChain.extent.height })
			.setImageArrayLayers(aWindow->mImageCreateInfoSwapChain.arrayLayers) // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are developing a stereoscopic 3D application. [2]
			.setImageUsage(aWindow->mImageCreateInfoSwapChain.usage)
			.setPreTransform(srfCaps.currentTransform) // To specify that you do not want any transformation, simply specify the current transformation. [2]
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque) // => no blending with other windows
			.setPresentMode(aWindow->get_config_presentation_mode(aWindow->surface()))
			.setClipped(VK_TRUE) // we don't care about the color of pixels that are obscured, for example because another window is in front of them.  [2]
			.setOldSwapchain({}) // TODO: This won't be enought, I'm afraid/pretty sure. => advanced chapter
			.setImageSharingMode(aWindow->mImageCreateInfoSwapChain.sharingMode)
			.setQueueFamilyIndexCount(aWindow->mImageCreateInfoSwapChain.queueFamilyIndexCount)
			.setPQueueFamilyIndices(aWindow->mImageCreateInfoSwapChain.pQueueFamilyIndices);

		// Finally, create the swap chain prepare a struct which stores all relevant data (for further use)
		aWindow->mSwapChain = device().createSwapchainKHRUnique(aWindow->mSwapChainCreateInfo);
		aWindow->mSwapChainImageFormat = surfaceFormat.format;
		aWindow->mSwapChainExtent = aWindow->mSwapChainCreateInfo.imageExtent;
		aWindow->mCurrentFrame = 0; // Start af frame 0

		auto swapChainImages = device().getSwapchainImagesKHR(aWindow->swap_chain());
		const auto imagesInFlight = swapChainImages.size();
		assert(imagesInFlight == aWindow->get_config_number_of_presentable_images());

		// and create one image view per image
		aWindow->mSwapChainImageViews.reserve(imagesInFlight);
		for (auto& imageHandle : swapChainImages) {
			// Note:: If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers. You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers. [3]
			auto& ref = aWindow->mSwapChainImageViews.emplace_back(create_image_view(wrap_image(imageHandle, aWindow->mImageCreateInfoSwapChain, aWindow->mImageUsage, vk::ImageAspectFlagBits::eColor)));
			ref.enable_shared_ownership(); // Back buffers must be in shared ownership, because they are also stored in the renderpass (see below), and imgui_manager will also require it that way if it is enabled.
		}

		// Create a renderpass for the back buffers
		std::vector<avk::attachment> renderpassAttachments = {
			avk::attachment::declare_for(const_referenced(aWindow->mSwapChainImageViews[0]), avk::on_load::clear, avk::color(0), avk::on_store::store)
		};
		auto additionalAttachments = aWindow->get_additional_back_buffer_attachments();
		renderpassAttachments.insert(std::end(renderpassAttachments), std::begin(additionalAttachments), std::end(additionalAttachments));
		aWindow->mBackBufferRenderpass = create_renderpass(renderpassAttachments);
		aWindow->mBackBufferRenderpass.enable_shared_ownership(); // Also shared ownership on this one... because... why noooot?

		// Create a back buffer per image
		aWindow->mBackBuffers.reserve(imagesInFlight);
		for (auto& imView: aWindow->mSwapChainImageViews) {
			auto imExtent = imView->get_image().config().extent;

			// Create one image view per attachment
			std::vector<avk::resource_ownership<avk::image_view_t>> imageViews;
			imageViews.reserve(renderpassAttachments.size());
			imageViews.push_back(avk::shared(imView)); // The color attachment is added in any case
			for (auto& aa : additionalAttachments) {
				if (aa.is_used_as_depth_stencil_attachment()) {
					auto depthView = create_depth_image_view(avk::owned(create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::read_only_depth_stencil_attachment))); // TODO: read_only_* or better general_*?
					imageViews.emplace_back(std::move(depthView));
				}
				else {
					imageViews.emplace_back(create_image_view(avk::owned(create_image(imExtent.width, imExtent.height, aa.format(), 1, avk::memory_usage::device, avk::image_usage::general_color_attachment))));
				}
			}

			aWindow->mBackBuffers.push_back(create_framebuffer(avk::shared(aWindow->mBackBufferRenderpass), std::move(imageViews), imExtent.width, imExtent.height));
			aWindow->mBackBuffers.back().enable_shared_ownership();
		}
		assert(aWindow->mBackBuffers.size() == imagesInFlight);

		// Transfer the backbuffer images into a at least somewhat useful layout for a start:
		for (auto& bb : aWindow->mBackBuffers) {
			const auto n = bb->image_views().size();
			assert(n == aWindow->get_renderpass()->number_of_attachment_descriptions());
			for (size_t i = 0; i < n; ++i) {
				bb->image_view_at(i)->get_image().transition_to_layout(aWindow->get_renderpass()->attachment_descriptions()[i].finalLayout, avk::sync::wait_idle(true));
			}
		}

		// ============= SYNCHRONIZATION OBJECTS ===========
		// per IMAGE:
		{
			aWindow->mImagesInFlightFenceIndices.reserve(imagesInFlight);
			for (uint32_t i = 0; i < imagesInFlight; ++i) {
				aWindow->mImagesInFlightFenceIndices.push_back(-1);
			}
		}
		assert(aWindow->mImagesInFlightFenceIndices.size() == imagesInFlight);

		// ============= SYNCHRONIZATION OBJECTS ===========
		// per CONCURRENT FRAME:
		{
			auto framesInFlight = aWindow->get_config_number_of_concurrent_frames();
			aWindow->mFramesInFlightFences.reserve(framesInFlight);
			aWindow->mImageAvailableSemaphores.reserve(framesInFlight);
			aWindow->mInitiatePresentSemaphores.reserve(framesInFlight);
			for (uint32_t i = 0; i < framesInFlight; ++i) {
				aWindow->mFramesInFlightFences.push_back(create_fence(true)); // true => Create the fences in signalled state, so that `cgb::context().logical_device().waitForFences` at the beginning of `window::render_frame` is not blocking forever, but can continue immediately.
				aWindow->mImageAvailableSemaphores.push_back(create_semaphore());
				aWindow->mInitiatePresentSemaphores.push_back(create_semaphore());
			}
		}
		assert(aWindow->mFramesInFlightFences.size() == aWindow->get_config_number_of_concurrent_frames());
		assert(aWindow->mImageAvailableSemaphores.size() == aWindow->get_config_number_of_concurrent_frames());
	}
}
