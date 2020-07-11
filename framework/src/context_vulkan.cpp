#include <exekutor.hpp>

#include <set>

namespace xk
{
	std::vector<const char*> vulkan::sRequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	std::mutex vulkan::sConcurrentAccessMutex;

	std::vector<const char*> vulkan::assemble_validation_layers()
	{
		std::vector<const char*> supportedValidationLayers;

		bool enableValidationLayers = settings::gEnableValidationLayersAlsoInReleaseBuilds;
#ifdef _DEBUG
		enableValidationLayers = true; // always true
#endif

		if (enableValidationLayers) {
			std::copy_if(
				std::begin(settings::gValidationLayersToBeActivated), std::end(settings::gValidationLayersToBeActivated),
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

	vulkan::vulkan()
		: generic_glfw()
		// Set a default for the requested physical device features:
		, mRequestedPhysicalDeviceFeatures { 
			vk::PhysicalDeviceFeatures()
				.setGeometryShader(VK_TRUE)
				.setTessellationShader(VK_TRUE)
				.setSamplerAnisotropy(VK_TRUE)
				.setVertexPipelineStoresAndAtomics(VK_TRUE)
				.setFragmentStoresAndAtomics(VK_TRUE)
				.setShaderStorageImageExtendedFormats(VK_TRUE)
				.setSampleRateShading(VK_TRUE)
		}
		// Set a default for the requested Vulkan 1.2 device features:
		, mRequestedVulkan12DeviceFeatures { 
			vk::PhysicalDeviceVulkan12Features()
				.setDescriptorBindingVariableDescriptorCount(VK_TRUE)
				.setRuntimeDescriptorArray(VK_TRUE)
				.setShaderUniformTexelBufferArrayDynamicIndexing(VK_TRUE)
				.setShaderStorageTexelBufferArrayDynamicIndexing(VK_TRUE)
				.setDescriptorIndexing(VK_TRUE)
		}
	{
		// So it begins
		create_instance();

#ifdef _DEBUG
		// Setup debug callback and enable all validation layers configured in global settings 
		setup_vk_debug_callback();

		if (std::find(std::begin(settings::gRequiredInstanceExtensions), std::end(settings::gRequiredInstanceExtensions), VK_EXT_DEBUG_REPORT_EXTENSION_NAME) != settings::gRequiredInstanceExtensions.end()) {
			setup_vk_debug_report_callback();
		}
#endif

		// The window surface needs to be created right after the instance creation 
		// and before physical device selection, because it can actually influence 
		// the physical device selection.

		mContextState = xk::context_state::initialization_begun;

		// NOTE: Vulkan-init is not finished yet!
		// Initialization will continue after the first window (and it's surface) have been created.
		// Only after the first window's surface has been created, the vulkan context can complete
		//   initialization and enter the context state of fully_initialized.
		//
		// Attention: May not use the `add_event_handler`-method here, because it would internally
		//   make use of `cgb::context()` which would refer to this static instance, which has not 
		//   yet finished initialization => would deadlock; Instead, modify data structure directly. 
		//   This constructor is the only exception, in all other cases, it's safe to use `add_event_handler`
		//   
		mEventHandlers.emplace_back([]() -> bool {
			LOG_DEBUG_VERBOSE("Running event handler to pick physical device");

			// Just get any window:
			auto* window = context().find_window([](xk::window* w) { 
				return w->handle().has_value() && static_cast<bool>(w->surface());
			});

			// Do we have a window with a handle?
			if (nullptr == window) { 
				return false; // Nope => not done
			}

			// We need a SURFACE to create the logical device => do it after the first window has been created
			auto& surface = window->surface();

			// Select the best suitable physical device which supports all requested extensions
			context().pick_physical_device(surface);

			return true;
		}, xk::context_state::initialization_begun);

		mEventHandlers.emplace_back([]() -> bool {
			LOG_DEBUG_VERBOSE("Running event handler to create logical device");

			// Just get any window:
			auto* window = context().find_window([](xk::window* w) { 
				return w->handle().has_value() && static_cast<bool>(w->surface());
			});

			// Do we have a window with a handle?
			if (nullptr == window) { 
				return false; // Nope => not done
			}

			// We need a SURFACE to create the logical device => do it after the first window has been created
			auto& surface = window->surface();

			// Do we already have a physical device?
			if (!context().physical_device()) {
				return false; // Nope => wait a bit longer
			}

			// Alright => let's move on and finally finish Vulkan initialization
			context().create_and_assign_logical_device(surface);

			return true;
		}, xk::context_state::physical_device_selected);
	}

	vulkan::~vulkan()
	{
		mContextState = xk::context_state::about_to_finalize;
		context().work_off_event_handlers();

		ak::sync::sPoolToAllocCommandBuffersFrom = ak::command_pool{};
		ak::sync::sQueueToUse = nullptr;
		
		// Destroy all command pools before the queues and the device is destroyed
		mCommandPools.clear();

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

		// Destroy logical device
		mLogicalDevice.destroy();

		// Unhook debug callback
#if LOG_LEVEL > 0
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(mInstance, mDebugUtilsCallbackHandle, nullptr); 
		}
#endif

		// Destroy everything
		mInstance.destroy();
	
		mContextState = xk::context_state::has_finalized;
		context().work_off_event_handlers();
	}

	void vulkan::check_vk_result(VkResult err)
	{
		createResultValue(static_cast<vk::Result>(err), context().vulkan_instance(), "check_vk_result");
	}

	ak::command_pool& vulkan::get_command_pool_for(uint32_t aQueueFamilyIndex, vk::CommandPoolCreateFlags aFlags)
	{
		std::scoped_lock<std::mutex> guard(sConcurrentAccessMutex);
		auto it = std::find_if(std::begin(mCommandPools), std::end(mCommandPools),
							   [lThreadId = std::this_thread::get_id(), lFamilyIdx = aQueueFamilyIndex, lFlags = aFlags] (const std::tuple<std::thread::id, ak::command_pool>& existing) {
									auto& tid = std::get<0>(existing);
									auto& q = std::get<1>(existing);
									return tid == lThreadId && q->queue_family_index() == lFamilyIdx && lFlags == q->create_info().flags;
							   });
		if (it == std::end(mCommandPools)) {
			return std::get<1>(mCommandPools.emplace_back(std::this_thread::get_id(), create_command_pool(aQueueFamilyIndex, aFlags)));
		}
		return std::get<1>(*it);
	}

	ak::command_pool& vulkan::get_command_pool_for(const ak::queue& aQueue, vk::CommandPoolCreateFlags aFlags)
	{
		return get_command_pool_for(aQueue.family_index(), aFlags);
	}

	ak::command_pool& vulkan::get_command_pool_for_single_use_command_buffers(const ak::queue& aQueue)
	{
		return get_command_pool_for(aQueue, vk::CommandPoolCreateFlagBits::eTransient);
	}
	
	ak::command_pool& vulkan::get_command_pool_for_reusable_command_buffers(const ak::queue& aQueue)
	{
		return get_command_pool_for(aQueue, {});
	}
	
	ak::command_pool& vulkan::get_command_pool_for_resettable_command_buffers(const ak::queue& aQueue)
	{
		return get_command_pool_for(aQueue, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
	}
	
	void vulkan::begin_composition()
	{ 
		dispatch_to_main_thread([]() {
			context().mContextState = xk::context_state::composition_beginning;
			context().work_off_event_handlers();
		});
	}

	void vulkan::end_composition()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = xk::context_state::composition_ending;
			context().work_off_event_handlers();
			
			context().mLogicalDevice.waitIdle();
		});
	}

	void vulkan::begin_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = xk::context_state::frame_begun;
			context().work_off_event_handlers();
		});
	}

	void vulkan::update_stage_done()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = xk::context_state::frame_updates_done;
			context().work_off_event_handlers();
		});
	}

	void vulkan::end_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = xk::context_state::frame_ended;
			context().work_off_event_handlers();
		});
	}

	window* vulkan::create_window(const std::string& _Title)
	{
		assert(are_we_on_the_main_thread());
		context().work_off_event_handlers();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		auto* wnd = generic_glfw::prepare_window();
		wnd->set_title(_Title);

		// Wait for the window to receive a valid handle before creating its surface
		context().add_event_handler(context_state::initialization_begun | context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running window surface creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](xk::window* w) {
				return w == wnd && w->handle().has_value();
			});

			if (nullptr == window) { // not yet
				return false;
			}

			VkSurfaceKHR surface;
			if (VK_SUCCESS != glfwCreateWindowSurface(context().vulkan_instance(), wnd->handle()->mHandle, nullptr, &surface)) {
				throw xk::runtime_error(fmt::format("Failed to create surface for window '{}'!", wnd->title()));
			}
			//window->mSurface = surface;
			vk::ObjectDestroy<vk::Instance, vk::DispatchLoaderStatic> deleter(context().vulkan_instance(), nullptr, vk::DispatchLoaderStatic());
			window->mSurface = vk::UniqueHandle<vk::SurfaceKHR, vk::DispatchLoaderStatic>(surface, deleter);
			return true;
		});

		// Continue with swap chain creation after the context has completely initialized
		//   and the window's handle and surface have been created
		context().add_event_handler(context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running swap chain creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](xk::window* w) { 
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

	void vulkan::create_instance()
	{
		// Information about the application for the instance creation call
		auto appInfo = vk::ApplicationInfo(settings::gApplicationName.c_str(), settings::gApplicationVersion,
										   "cg_base", VK_MAKE_VERSION(0, 1, 0), // TODO: Real version of cg_base
										   VK_API_VERSION_1_1);

		// GLFW requires several extensions to interface with the window system. Query them.
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		std::vector<const char*> requiredExtensions;
		requiredExtensions.assign(glfwExtensions, static_cast<const char**>(glfwExtensions + glfwExtensionCount));
		requiredExtensions.insert(
			std::end(requiredExtensions),
			std::begin(settings::gRequiredInstanceExtensions), std::end(settings::gRequiredInstanceExtensions));

		// Check for each validation layer if it exists and activate all which do.
		std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
		// Enable extension to receive callbacks for the validation layers
		if (supportedValidationLayers.size() > 0) {
			requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		// Gather all previously prepared info for instance creation and put in one struct:
		auto instCreateInfo = vk::InstanceCreateInfo()
			.setPApplicationInfo(&appInfo)
			.setEnabledExtensionCount(static_cast<uint32_t>(requiredExtensions.size()))
			.setPpEnabledExtensionNames(requiredExtensions.data())
			.setEnabledLayerCount(static_cast<uint32_t>(supportedValidationLayers.size()))
			.setPpEnabledLayerNames(supportedValidationLayers.data());
		// Create it, errors will result in an exception.
		mInstance = vk::createInstance(instCreateInfo);
		mDynamicDispatch.init((VkInstance)mInstance, vkGetInstanceProcAddr);
	}

	bool vulkan::is_validation_layer_supported(const char* pName)
	{
		auto availableLayers = vk::enumerateInstanceLayerProperties();
		return availableLayers.end() !=  std::find_if(
			std::begin(availableLayers), std::end(availableLayers), 
			[toFind = std::string(pName)](const vk::LayerProperties& e) {
				return std::string(static_cast<const char*>(e.layerName)) == toFind;
			});
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL vulkan::vk_debug_utils_callback(
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

	void vulkan::setup_vk_debug_callback()
	{
		assert(mInstance);
		// Configure logging
#if LOG_LEVEL > 0
		if (settings::gValidationLayersToBeActivated.size() == 0) {
			return;
		}

		auto msgCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT()
			.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
#if LOG_LEVEL > 1
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
#if LOG_LEVEL > 2
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo
#if LOG_LEVEL > 3
				| vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
#endif
#endif
#endif
			)
			.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
			.setPfnUserCallback(vulkan::vk_debug_utils_callback);

		// Hook in
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)mInstance.getProcAddr("vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			auto result = func(
				mInstance, 
				&static_cast<VkDebugUtilsMessengerCreateInfoEXT>(msgCreateInfo), 
				nullptr, 
				&mDebugUtilsCallbackHandle);
			if (VK_SUCCESS != result) {
				throw xk::runtime_error("Failed to set up debug utils callback via vkCreateDebugUtilsMessengerEXT");
			}
		}
		else {
			throw xk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugUtilsMessengerEXT.");
		}
#endif
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL vulkan::vk_debug_report_callback(
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

	void vulkan::setup_vk_debug_report_callback()
	{
		assert(mInstance);
#if LOG_LEVEL > 0
		auto createInfo = vk::DebugReportCallbackCreateInfoEXT{}
			.setFlags(
				vk::DebugReportFlagBitsEXT::eError
#if LOG_LEVEL > 1
				| vk::DebugReportFlagBitsEXT::eWarning | vk::DebugReportFlagBitsEXT::ePerformanceWarning
#if LOG_LEVEL > 2
				| vk::DebugReportFlagBitsEXT::eInformation
#endif
#if defined(_DEBUG)
				| vk::DebugReportFlagBitsEXT::eDebug
#endif
#endif
			)
			.setPfnCallback(vulkan::vk_debug_report_callback);
		
		// Hook in
		auto func = (PFN_vkCreateDebugReportCallbackEXT)mInstance.getProcAddr("vkCreateDebugReportCallbackEXT");
		if (func != nullptr) {
			auto result = func(
				mInstance,
				&static_cast<VkDebugReportCallbackCreateInfoEXT>(createInfo),
				nullptr,
				&mDebugReportCallbackHandle);
			if (VK_SUCCESS != result) {
				throw xk::runtime_error("Failed to set up debug report callback via vkCreateDebugReportCallbackEXT");
			}
		}
		else {
			throw xk::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugReportCallbackEXT.");
		}

#endif
	}

	std::vector<const char*> vulkan::get_all_required_device_extensions()
	{
		std::vector<const char*> combined;
		combined.assign(std::begin(settings::gRequiredDeviceExtensions), std::end(settings::gRequiredDeviceExtensions));
		combined.insert(std::end(combined), std::begin(sRequiredDeviceExtensions), std::end(sRequiredDeviceExtensions));
		return combined;
	}

	bool vulkan::supports_shading_rate_image(const vk::PhysicalDevice& device)
	{
		vk::PhysicalDeviceFeatures2 supportedExtFeatures;
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV();
		supportedExtFeatures.pNext = &shadingRateImageFeatureNV;
		device.getFeatures2(&supportedExtFeatures);
		return shadingRateImageFeatureNV.shadingRateImage && shadingRateImageFeatureNV.shadingRateCoarseSampleOrder && supportedExtFeatures.features.shaderStorageImageExtendedFormats;
	}

	bool vulkan::shading_rate_image_extension_requested()
	{
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		return std::find(std::begin(allRequiredDeviceExtensions), std::end(allRequiredDeviceExtensions), VK_NV_SHADING_RATE_IMAGE_EXTENSION_NAME) != std::end(allRequiredDeviceExtensions);
	}

	bool vulkan::ray_tracing_extension_requested()
	{
		auto allRequiredDeviceExtensions = get_all_required_device_extensions();
		return std::find(std::begin(allRequiredDeviceExtensions), std::end(allRequiredDeviceExtensions), VK_KHR_RAY_TRACING_EXTENSION_NAME) != std::end(allRequiredDeviceExtensions);
	}

	bool vulkan::supports_all_required_extensions(const vk::PhysicalDevice& device)
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

	void vulkan::pick_physical_device(vk::SurfaceKHR pSurface)
	{
		assert(mInstance);
		auto devices = mInstance.enumeratePhysicalDevices();
		if (devices.size() == 0) {
			throw xk::runtime_error("Failed to find GPUs with Vulkan support.");
		}
		const vk::PhysicalDevice* currentSelection = nullptr;
		uint32_t currentScore = 0; // device score
		
		// Iterate over all devices
		for (const auto& device : devices) {
			// get features and queues
			auto properties = device.getProperties();
			auto supportedFeatures = device.getFeatures();
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

			auto deviceName = std::string(static_cast<const char*>(properties.deviceName));
			
			if (!settings::gPhysicalDeviceSelectionHint.empty()) {
				score += ak::find_case_insensitive(deviceName, settings::gPhysicalDeviceSelectionHint, 0) != std::string::npos ? 1000 : 0;
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
			if (settings::gRequiredDeviceExtensions.size() > 0) {
				throw xk::runtime_error("Could not find a suitable physical device, most likely because no device supported all required device extensions.");
			}
			throw xk::runtime_error("Could not find a suitable physical device.");
		}

		// Handle success:
		mPhysicalDevice = *currentSelection;
		mContextState = xk::context_state::physical_device_selected;
		LOG_INFO(fmt::format("Going to use {}", mPhysicalDevice.getProperties().deviceName));
	}

	void vulkan::create_and_assign_logical_device(vk::SurfaceKHR pSurface)
	{
		assert(mPhysicalDevice);

		// Determine which queue families we have, i.e. what the different queue families support and what they don't
		auto familyIndex = ak::queue::select_queue_family_index(mPhysicalDevice, {}, ak::queue_selection_preference::versatile_queue, pSurface);
		auto iAmTheOneAndOnly = ak::queue::prepare(mPhysicalDevice, familyIndex, 0);
		auto queuesToBeCreated = std::vector<ak::queue>{ std::move(iAmTheOneAndOnly) };
		
		context().mContextState = xk::context_state::queues_prepared;
		
		LOG_DEBUG_VERBOSE("Running vulkan create_and_assign_logical_device event handler");

		// Get the same validation layers as for the instance!
		std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
	
		// Always prepare the shading rate image features descriptor, but only use it if the extension has been requested
		auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV()
			.setShadingRateImage(VK_TRUE)
			.setShadingRateCoarseSampleOrder(VK_TRUE);
		auto activateShadingRateImage = shading_rate_image_extension_requested() && supports_shading_rate_image(context().physical_device());

		auto queueCreateInfos = ak::queue::get_queue_config_for_DeviceCreateInfo(std::begin(queuesToBeCreated), std::end(queuesToBeCreated));
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

		if (xk::settings::gEnableBufferDeviceAddress) {
			deviceVulkan12Features.setBufferDeviceAddress(VK_TRUE);
		}

	    auto deviceRayTracingFeatures = vk::PhysicalDeviceRayTracingFeaturesKHR{};
		if (ray_tracing_extension_requested()) {
			deviceVulkan12Features.setBufferDeviceAddress(VK_TRUE);
			deviceRayTracingFeatures
				.setPNext(&deviceFeatures)
				.setRayTracing(VK_TRUE);
			deviceVulkan12Features.setPNext(&deviceRayTracingFeatures);
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
		context().mDynamicDispatch = vk::DispatchLoaderDynamic(
			context().vulkan_instance(), 
			vkGetInstanceProcAddr, // TODO: <-- Is this the right choice? There's also glfwGetInstanceProcAddress.. just saying.
			context().device()
		);

		// For every queue, assign the proper handle
		for (auto& q : queuesToBeCreated) {
			q.assign_handle(context().mLogicalDevice);
		}
		
		context().mPresentQueue		= queuesToBeCreated[0];
		context().mGraphicsQueue	= queuesToBeCreated[0];
		context().mComputeQueue		= queuesToBeCreated[0];
		context().mTransferQueue	= queuesToBeCreated[0];

		// Determine distinct queue family indices and distinct (family-id, queue-id)-tuples:
		std::set<uint32_t> uniqueFamilyIndices;
		std::set<std::tuple<uint32_t, uint32_t>> uniqueQueues;
		for (auto& q : queuesToBeCreated) {
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
		ak::sync::sPoolToAllocCommandBuffersFrom = xk::context().create_command_pool(xk::context().graphics_queue().family_index(), {});
		ak::sync::sQueueToUse = &xk::context().graphics_queue();
		
		context().mContextState = xk::context_state::fully_initialized;

	}

	void vulkan::create_swap_chain_for_window(window* pWindow)
	{
		auto srfCaps = mPhysicalDevice.getSurfaceCapabilitiesKHR(pWindow->surface());

		// Vulkan tells us to match the resolution of the window by setting the width and height in the 
		// currentExtent member. However, some window managers do allow us to differ here and this is 
		// indicated by setting the width and height in currentExtent to a special value: the maximum 
		// value of uint32_t. In that case we'll pick the resolution that best matches the window within 
		// the minImageExtent and maxImageExtent bounds. [2]
		auto extent = srfCaps.currentExtent.width == std::numeric_limits<uint32_t>::max()
			? glm::clamp(pWindow->resolution(),
						 glm::uvec2(srfCaps.minImageExtent.width, srfCaps.minImageExtent.height),
						 glm::uvec2(srfCaps.maxImageExtent.width, srfCaps.maxImageExtent.height))
			: glm::uvec2(srfCaps.currentExtent.width, srfCaps.currentExtent.height);

		auto surfaceFormat = pWindow->get_config_surface_format(pWindow->surface());

		const ak::image_usage swapChainImageUsage = ak::image_usage::color_attachment			 | ak::image_usage::transfer_destination		| ak::image_usage::presentable;
		const vk::ImageUsageFlags swapChainImageUsageVk =	vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
		pWindow->mImageCreateInfoSwapChain = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFormat(surfaceFormat.format)
			.setExtent(vk::Extent3D(extent.x, extent.y))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(swapChainImageUsageVk)
			.setInitialLayout(vk::ImageLayout::eUndefined);

		if (mPresentQueue == mGraphicsQueue) {
			// Found a queue family which supports both!
			// If the graphics queue family and presentation queue family are the same, which will be the case on most hardware, then we should stick to exclusive mode. [2]
			pWindow->mImageCreateInfoSwapChain
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0) // Optional [2]
				.setPQueueFamilyIndices(nullptr); // Optional [2]
		}
		else {
			pWindow->mQueueFamilyIndices.push_back(mPresentQueue.family_index());
			pWindow->mQueueFamilyIndices.push_back(mGraphicsQueue.family_index());
			// Have to use separate queue families!
			// If the queue families differ, then we'll be using the concurrent mode [2]
			pWindow->mImageCreateInfoSwapChain
				.setSharingMode(vk::SharingMode::eConcurrent)
				.setQueueFamilyIndexCount(static_cast<uint32_t>(pWindow->mQueueFamilyIndices.size()))
				.setPQueueFamilyIndices(pWindow->mQueueFamilyIndices.data());
		}

		// With all settings gathered, create the swap chain!
		auto createInfo = vk::SwapchainCreateInfoKHR()
			.setSurface(pWindow->surface())
			.setMinImageCount(pWindow->get_config_number_of_presentable_images())
			.setImageFormat(pWindow->mImageCreateInfoSwapChain.format)
			.setImageColorSpace(surfaceFormat.colorSpace)
			.setImageExtent(vk::Extent2D{ pWindow->mImageCreateInfoSwapChain.extent.width, pWindow->mImageCreateInfoSwapChain.extent.height })
			.setImageArrayLayers(pWindow->mImageCreateInfoSwapChain.arrayLayers) // The imageArrayLayers specifies the amount of layers each image consists of. This is always 1 unless you are developing a stereoscopic 3D application. [2]
			.setImageUsage(pWindow->mImageCreateInfoSwapChain.usage)
			.setPreTransform(srfCaps.currentTransform) // To specify that you do not want any transformation, simply specify the current transformation. [2]
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque) // => no blending with other windows
			.setPresentMode(pWindow->get_config_presentation_mode(pWindow->surface()))
			.setClipped(VK_TRUE) // we don't care about the color of pixels that are obscured, for example because another window is in front of them.  [2]
			.setOldSwapchain({}) // TODO: This won't be enought, I'm afraid/pretty sure. => advanced chapter
			.setImageSharingMode(pWindow->mImageCreateInfoSwapChain.sharingMode)
			.setQueueFamilyIndexCount(pWindow->mImageCreateInfoSwapChain.queueFamilyIndexCount)
			.setPQueueFamilyIndices(pWindow->mImageCreateInfoSwapChain.pQueueFamilyIndices);

		// Finally, create the swap chain prepare a struct which stores all relevant data (for further use)
		pWindow->mSwapChain = device().createSwapchainKHRUnique(createInfo);
		//pWindow->mSwapChain = logical_device().createSwapchainKHR(createInfo);
		pWindow->mSwapChainImageFormat = surfaceFormat.format;
		pWindow->mSwapChainExtent = vk::Extent2D(extent.x, extent.y);
		pWindow->mCurrentFrame = 0; // Start af frame 0

		auto swapChainImages = device().getSwapchainImagesKHR(pWindow->swap_chain());
		assert(swapChainImages.size() == pWindow->get_config_number_of_presentable_images());

		// and create one image view per image
		pWindow->mSwapChainImageViews.reserve(swapChainImages.size());
		for (auto& imageHandle : swapChainImages) {
			// Note:: If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers. You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers. [3]
			auto& ref = pWindow->mSwapChainImageViews.emplace_back(create_image_view(wrap_image(imageHandle, pWindow->mImageCreateInfoSwapChain, swapChainImageUsage, vk::ImageAspectFlagBits::eColor)));
			ref.enable_shared_ownership(); // Back buffers must be in shared ownership, because they are also stored in the renderpass (see below), and imgui_manager will also require it that way if it is enabled.
		}

		// Create a renderpass for the back buffers
		std::vector<ak::attachment> renderpassAttachments = {
			ak::attachment::declare_for(pWindow->mSwapChainImageViews[0], ak::on_load::clear, ak::color(0), ak::on_store::dont_care)
		};
		auto additionalAttachments = pWindow->get_additional_back_buffer_attachments();
		renderpassAttachments.insert(std::end(renderpassAttachments), std::begin(additionalAttachments), std::end(additionalAttachments)),
		pWindow->mBackBufferRenderpass = create_renderpass(renderpassAttachments);
		pWindow->mBackBufferRenderpass.enable_shared_ownership(); // Also shared ownership on this one... because... why noooot?

		// Create a back buffer per image
		pWindow->mBackBuffers.reserve(pWindow->mSwapChainImageViews.size());
		for (auto& imView: pWindow->mSwapChainImageViews) {
			auto imExtent = imView->get_image().config().extent;

			// Create one image view per attachment
			std::vector<ak::image_view> imageViews;
			imageViews.reserve(renderpassAttachments.size());
			imageViews.push_back(imView); // The color attachment is added in any case
			for (auto& aa : additionalAttachments) {
				if (aa.is_used_as_depth_stencil_attachment()) {
					auto depthView = create_depth_image_view(create_image(imExtent.width, imExtent.height, aa.format(), 1, ak::memory_usage::device, ak::image_usage::read_only_depth_stencil_attachment)); // TODO: read_only_* or better general_*?
					imageViews.emplace_back(std::move(depthView));
				}
				else {
					imageViews.emplace_back(create_image_view(create_image(imExtent.width, imExtent.height, aa.format(), 1, ak::memory_usage::device, ak::image_usage::general_color_attachment)));
				}
			}

			pWindow->mBackBuffers.push_back(create_framebuffer(pWindow->mBackBufferRenderpass, std::move(imageViews), imExtent.width, imExtent.height));
		}
		assert(pWindow->mBackBuffers.size() == pWindow->get_config_number_of_presentable_images());

		// Transfer the backbuffer images into a at least somewhat useful layout for a start:
		for (auto& bb : pWindow->mBackBuffers) {
			const auto n = bb->image_views().size();
			assert(n == pWindow->get_renderpass().attachment_descriptions().size());
			for (size_t i = 0; i < n; ++i) {
				bb->image_view_at(i)->get_image().transition_to_layout(pWindow->get_renderpass().attachment_descriptions()[i].finalLayout, ak::sync::wait_idle(true));
			}
		}

		// ============= SYNCHRONIZATION OBJECTS ===========
		{
			const auto n = pWindow->get_config_number_of_presentable_images();
			pWindow->mImageAvailableSemaphores.reserve(n);
			for (uint32_t i = 0; i < n; ++i) {
				pWindow->mImageAvailableSemaphores.push_back(create_semaphore());
			}
		}
		assert(pWindow->mImageAvailableSemaphores.size() == pWindow->get_config_number_of_presentable_images());
		
		{
			auto n = pWindow->get_config_number_of_concurrent_frames();
			pWindow->mFences.reserve(n);
			pWindow->mRenderFinishedSemaphores.reserve(n);
			for (uint32_t i = 0; i < n; ++i) {
				pWindow->mFences.push_back(create_fence(true)); // true => Create the fences in signalled state, so that `cgb::context().logical_device().waitForFences` at the beginning of `window::render_frame` is not blocking forever, but can continue immediately.
				pWindow->mRenderFinishedSemaphores.push_back(create_semaphore());
			}
		}
		assert(pWindow->mFences.size() == pWindow->get_config_number_of_concurrent_frames());
		assert(pWindow->mRenderFinishedSemaphores.size() == pWindow->get_config_number_of_concurrent_frames());
	}
}
