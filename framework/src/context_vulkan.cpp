#include <cg_base.hpp>

#include <set>

namespace cgb
{
	std::vector<const char*> vulkan::sRequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	auto vulkan::assemble_validation_layers()
	{
		std::vector<const char*> supportedValidationLayers;
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
		return supportedValidationLayers;
	}

	vulkan::vulkan() : generic_glfw()
	{
		// So it begins
		create_instance();

		// Setup debug callback and enable all validation layers configured in global settings 
		setup_vk_debug_callback();

		// The window surface needs to be created right after the instance creation 
		// and before physical device selection, because it can actually influence 
		// the physical device selection.

		mContextState = cgb::context_state::halfway_initialized;

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
			auto* window = context().find_window([](cgb::window* w) { 
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
		}, cgb::context_state::halfway_initialized);

		mEventHandlers.emplace_back([]() -> bool {
			LOG_DEBUG_VERBOSE("Running event handler to create logical device");

			// Just get any window:
			auto* window = context().find_window([](cgb::window* w) { 
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
		}, cgb::context_state::halfway_initialized);

		// We're still not done yet with our initial setup efforts if we need ImGui,
		// but we're going to set that up at an even later point -> when we have the 
		// context fully initiylized:
		if (!settings::gDisableImGui) {
			// Init and wire-in IMGUI
			//
			// Attention: May not use the `add_event_handler`-method here, because it would internally
			//   make use of `cgb::context()` which would refer to this static instance, which has not 
			//   yet finished initialization => would deadlock; Instead, modify data structure directly. 
			//   This constructor is the only exception, in all other cases, it's safe to use `add_event_handler`
			//   
			mEventHandlers.emplace_back([]() -> bool {
				LOG_DEBUG_VERBOSE("Running IMGUI setup event handler");

				// Just get any window:
				auto* window = context().find_window([](cgb::window* w) { 
					return w->handle().has_value();
				});

				// Do we have a window with a handle?
				if (nullptr == window) { 
					return false; // Nope => not done
				}

				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO(); (void)io;
				//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
				//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

				// Setup Dear ImGui style
				ImGui::StyleColorsDark();
				//ImGui::StyleColorsClassic();

				// Setup Platform/Renderer bindings
				ImGui_ImplGlfw_InitForVulkan(window->handle()->mHandle, true); // TODO: Don't install callbacks

				//struct ImGui_ImplVulkan_InitInfo
				//{
				//	VkInstance          Instance;
				//	VkPhysicalDevice    PhysicalDevice;
				//	VkDevice            Device;
				//	uint32_t            QueueFamily;
				//	VkQueue             Queue;
				//	VkPipelineCache     PipelineCache;
				//	VkDescriptorPool    DescriptorPool;
				//	uint32_t            MinImageCount;          // >= 2
				//	uint32_t            ImageCount;             // >= MinImageCount
				//	const VkAllocationCallbacks* Allocator;
				//	void                (*CheckVkResultFn)(VkResult err);
				//};

				ImGui_ImplVulkan_InitInfo init_info = {};
				init_info.Instance = context().vulkan_instance();
				init_info.PhysicalDevice = context().physical_device();
				init_info.Device = context().logical_device();;
				init_info.QueueFamily = context().graphics_queue_index();
				init_info.Queue = context().graphics_queue().handle();
				init_info.PipelineCache = VK_NULL_HANDLE;
				//init_info.DescriptorPool = context().get_descriptor_pool().mDescriptorPool; // TODO: give ImGui a suitable descriptor pool!
				init_info.Allocator = VK_NULL_HANDLE;
				//init_info.MinImageCount = sActualMaxFramesInFlight; // <---- TODO
				//init_info.ImageCount = sActualMaxFramesInFlight; // <---- TODO
				init_info.CheckVkResultFn = check_vk_result;

				// TODO: Hmm, or do we have to do this per swap chain? 
				return true;
			}, cgb::context_state::fully_initialized);
		}
	}

	vulkan::~vulkan()
	{
		mContextState = cgb::context_state::about_to_finalize;
		context().work_off_event_handlers();

		// Destroy all descriptor pools before the queues and the device is destroyed
		mWorkaroundDescriptorPool.reset();
		mWorkaroundDescriptorPool.release();
		mDescriptorPools.clear();

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
			func(mInstance, mDebugCallbackHandle, nullptr); 
		}
#endif

		// Destroy everything
		mInstance.destroy();
	
		mContextState = cgb::context_state::has_finalized;
		context().work_off_event_handlers();
	}

	void vulkan::check_vk_result(VkResult err)
	{
		createResultValue(static_cast<vk::Result>(err), context().vulkan_instance(), "check_vk_result");
	}

	void vulkan::begin_composition()
	{ 
		dispatch_to_main_thread([]() {
			context().mContextState = cgb::context_state::composition_beginning;
			context().work_off_event_handlers();
		});
	}

	void vulkan::end_composition()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = cgb::context_state::composition_ending;
			context().work_off_event_handlers();
			context().mLogicalDevice.waitIdle();
		});
	}

	void vulkan::begin_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = cgb::context_state::frame_begun;
			context().work_off_event_handlers();
		});
	}

	void vulkan::update_stage_done()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = cgb::context_state::frame_updates_done;
			context().work_off_event_handlers();
		});
	}

	void vulkan::end_frame()
	{
		dispatch_to_main_thread([]() {
			context().mContextState = cgb::context_state::frame_ended;
			context().work_off_event_handlers();
		});
	}

	void vulkan::draw_triangle(const graphics_pipeline_t& pPipeline, const command_buffer& pCommandBuffer)
	{
		pCommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, pPipeline.handle());
		pCommandBuffer.handle().draw(3u, 1u, 0u, 0u);
	}


	window* vulkan::create_window(const std::string& pTitle)
	{
		assert(are_we_on_the_main_thread());
		context().work_off_event_handlers();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		auto* wnd = generic_glfw::prepare_window();

		// Wait for the window to receive a valid handle before creating its surface
		context().add_event_handler(context_state::halfway_initialized | context_state::anytime_after_init_before_finalize, [wnd]() -> bool {
			LOG_DEBUG_VERBOSE("Running window surface creator event handler");

			// Make sure it is the right window
			auto* window = context().find_window([wnd](cgb::window* w) {
				return w == wnd && w->handle().has_value();
			});

			if (nullptr == window) { // not yet
				return false;
			}

			VkSurfaceKHR surface;
			if (VK_SUCCESS != glfwCreateWindowSurface(context().vulkan_instance(), wnd->handle()->mHandle, nullptr, &surface)) {
				throw std::runtime_error(fmt::format("Failed to create surface for window '{}'!", wnd->title()));
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
			auto* window = context().find_window([wnd](cgb::window* w) { 
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
				return e.layerName == toFind;
			});
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL vulkan::vk_debug_callback(
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
			LOG_ERROR__(fmt::format("Vk-callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber, 
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			assert(pCallbackData);
			LOG_WARNING__(fmt::format("Vk-callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
			assert(pCallbackData);
			if (std::string("Loader Message") == pCallbackData->pMessageIdName) {
				LOG_VERBOSE__(fmt::format("Vk-callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					pCallbackData->pMessageIdName,
					pCallbackData->pMessage));
			}
			else {
				LOG_INFO__(fmt::format("Vk-callback with Id[{}|{}] and Message[{}]",
					pCallbackData->messageIdNumber,
					pCallbackData->pMessageIdName,
					pCallbackData->pMessage));
			}
		}
		else if (pMessageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
			assert(pCallbackData);
			LOG_VERBOSE__(fmt::format("Vk-callback with Id[{}|{}] and Message[{}]",
				pCallbackData->messageIdNumber,
				pCallbackData->pMessageIdName,
				pCallbackData->pMessage));
		}
		return VK_FALSE; 
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
			.setPfnUserCallback(vulkan::vk_debug_callback);

		// Hook in
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			auto result = func(
				mInstance, 
				&static_cast<VkDebugUtilsMessengerCreateInfoEXT>(msgCreateInfo), 
				nullptr, 
				&mDebugCallbackHandle);
			if (VK_SUCCESS != result) {
				throw std::runtime_error("Failed to set up debug callback via vkCreateDebugUtilsMessengerEXT");
			}
		}
		else {
			throw std::runtime_error("Failed to vkGetInstanceProcAddr for vkCreateDebugUtilsMessengerEXT.");
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
			throw std::runtime_error("Failed to find GPUs with Vulkan support.");
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

			// TODO: Prioritizing nvidia is a bad solution, of course. 
			// It is/was useful during development, but should be replaced by some meaningful code:
			uint32_t score =
				(graphicsBitSet ? 10 : 0) +
				(computeBitSet ? 10 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? 10 : 0) +
				(properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu ? 5 : 0) +
				(find_case_insensitive(properties.deviceName, "nvidia", 0) != std::string::npos ? 1 : 0);

			// Check if extensions are required
			if (!supports_all_required_extensions(device)) {
				score = 0;
			}

			// Check if anisotropy is supported
			if (!supportedFeatures.samplerAnisotropy) {
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
				throw std::runtime_error("Could not find a suitable physical device, most likely because no device supported all required device extensions.");
			}
			throw std::runtime_error("Could not find a suitable physical device.");
		}

		// Handle success:
		mPhysicalDevice = *currentSelection;
	}

	std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> vulkan::find_queue_families_for_criteria(
		vk::QueueFlags pRequiredFlags, 
		vk::QueueFlags pForbiddenFlags, 
		std::optional<vk::SurfaceKHR> pSurface)
	{
		assert(mPhysicalDevice);
		// All queue families:
		auto queueFamilies = physical_device().getQueueFamilyProperties();
		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> indexedQueueFamilies;
		std::transform(std::begin(queueFamilies), std::end(queueFamilies),
					   std::back_inserter(indexedQueueFamilies),
					   [index = uint32_t(0)](const decltype(queueFamilies)::value_type& input) mutable {
						   auto tpl = std::make_tuple(index, input);
						   index += 1;
						   return tpl;
					   });
		// Subset to which the criteria applies:
		std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> selection;
		// Select the subset
		std::copy_if(std::begin(indexedQueueFamilies), std::end(indexedQueueFamilies),
					 std::back_inserter(selection),
					 [pRequiredFlags, pForbiddenFlags, pSurface, this](const std::tuple<uint32_t, decltype(queueFamilies)::value_type>& tpl) {
						 bool requirementsMet = true;
						 if (pRequiredFlags) {
							 requirementsMet = requirementsMet && ((std::get<1>(tpl).queueFlags & pRequiredFlags) == pRequiredFlags);
						 }
						 if (pForbiddenFlags) {
							 requirementsMet = requirementsMet && ((std::get<1>(tpl).queueFlags & pForbiddenFlags) != pForbiddenFlags);
						 }
						 if (pSurface) {
							 requirementsMet = requirementsMet && (mPhysicalDevice.getSurfaceSupportKHR(std::get<0>(tpl), *pSurface));
						 }
						 return requirementsMet;
					 });
		return selection;
	}

	std::vector<std::tuple<uint32_t, vk::QueueFamilyProperties>> vulkan::find_best_queue_family_for(vk::QueueFlags pRequiredFlags, device_queue_selection_strategy pSelectionStrategy, std::optional<vk::SurfaceKHR> pSurface)
	{
		static std::array queueTypes = {
			vk::QueueFlagBits::eGraphics,
			vk::QueueFlagBits::eCompute,
			vk::QueueFlagBits::eTransfer,
		};
		// sparse binding and protected bits are ignored, for now

		decltype(std::declval<vulkan>().find_queue_families_for_criteria(vk::QueueFlags(), vk::QueueFlags(), std::nullopt)) selection;
		
		vk::QueueFlags forbiddenFlags = {};
		for (auto f : queueTypes) {
			forbiddenFlags |= f;
		}
		forbiddenFlags &= ~pRequiredFlags;

		int32_t loosenIndex = 0;
		
		switch (pSelectionStrategy) {
		case cgb::device_queue_selection_strategy::prefer_separate_queues:
			while (loosenIndex <= queueTypes.size()) { // might result in returning an empty selection
				selection = find_queue_families_for_criteria(pRequiredFlags, forbiddenFlags, pSurface);
				if (selection.size() > 0 || loosenIndex == queueTypes.size()) {
					break;
				}
				forbiddenFlags = forbiddenFlags & ~queueTypes[loosenIndex++]; // gradually loosen restrictions
			};
			break;
		case cgb::device_queue_selection_strategy::prefer_everything_on_single_queue:
			// Nothing is forbidden
			selection = find_queue_families_for_criteria(pRequiredFlags, vk::QueueFlags(), pSurface);
			break;
		}

		return selection;
	}

	void vulkan::create_and_assign_logical_device(vk::SurfaceKHR pSurface)
	{
		assert(mPhysicalDevice);

		// Determine which queue families we have, i.e. what the different queue families support and what they don't
		auto* presentQueue		= device_queue::prepare(vk::QueueFlagBits::eGraphics,		settings::gQueueSelectionPreference, pSurface);
		auto* graphicsQueue		= device_queue::prepare(vk::QueueFlagBits::eGraphics,		settings::gPreferSameQueueForGraphicsAndPresent 
																							  ? device_queue_selection_strategy::prefer_everything_on_single_queue
																							  : settings::gQueueSelectionPreference, std::nullopt);
		auto* computeQueue		= device_queue::prepare(vk::QueueFlagBits::eCompute,		settings::gQueueSelectionPreference, std::nullopt);
		auto* transferQueue		= device_queue::prepare(vk::QueueFlagBits::eTransfer,		settings::gQueueSelectionPreference, std::nullopt);

		// Defer pipeline creation to enable the user to request more queues

		mEventHandlers.emplace_back([presentQueue, graphicsQueue, computeQueue, transferQueue]() -> bool {
			LOG_DEBUG_VERBOSE("Running vulkan create_and_assign_logical_device event handler");

			// Get the same validation layers as for the instance!
			std::vector<const char*> supportedValidationLayers = assemble_validation_layers();
		
			// Always prepare the shading rate image features descriptor, but only use it if the extension has been requested
			auto shadingRateImageFeatureNV = vk::PhysicalDeviceShadingRateImageFeaturesNV()
				.setShadingRateImage(VK_TRUE)
				.setShadingRateCoarseSampleOrder(VK_TRUE);
			auto activateShadingRateImage = shading_rate_image_extension_requested() && supports_shading_rate_image(context().physical_device());

			// Gather all the queue create infos:
			std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
			std::vector<std::vector<float>> queuePriorities;
			for (size_t q = 0; q < device_queue::sPreparedQueues.size(); ++q) {
				auto& pq = device_queue::sPreparedQueues[q];
				bool handled = false;
				for (size_t i = 0; i < queueCreateInfos.size(); ++i) {
					if (queueCreateInfos[i].queueFamilyIndex == pq.family_index()) {
						// found, i.e. increase count IF it has a different queue index as all before
						bool isDifferent = true;
						for (size_t v = 0; v < q; ++v) {
							if (device_queue::sPreparedQueues[v].family_index() == pq.family_index()
								&& device_queue::sPreparedQueues[v].queue_index() == pq.queue_index()) {
								isDifferent = false;
							}
						}
						if (isDifferent) {
							queueCreateInfos[i].queueCount += 1;
							queuePriorities[i].push_back(pq.priority());
						}
						handled = true; 
						break; // done
					}
				}
				if (!handled) { // => must be a new entry
					queueCreateInfos.emplace_back()
						.setQueueFamilyIndex(pq.family_index())
						.setQueueCount(1u);
					queuePriorities.push_back({ pq.priority() });
				}
			}
			// Iterate over all vk::DeviceQueueCreateInfo entries and set the queue priorities pointers properly
			for (auto i = 0; i < queueCreateInfos.size(); ++i) {
				queueCreateInfos[i].setPQueuePriorities(queuePriorities[i].data());
			}

			// Enable certain device features:
			// (Build a pNext chain for further supported extensions)

			auto deviceFeatures = vk::PhysicalDeviceFeatures2()
				.setFeatures(vk::PhysicalDeviceFeatures()
					.setGeometryShader(VK_TRUE)
					.setSamplerAnisotropy(VK_TRUE)
					.setVertexPipelineStoresAndAtomics(VK_TRUE)
					.setFragmentStoresAndAtomics(VK_TRUE)
					.setShaderStorageImageExtendedFormats(VK_TRUE))
				.setPNext(activateShadingRateImage ? &shadingRateImageFeatureNV : nullptr);

			//Require variable descriptor count
			auto descriptorIndexingFeatures = vk::PhysicalDeviceDescriptorIndexingFeaturesEXT{}
				.setDescriptorBindingVariableDescriptorCount(VK_TRUE)
				.setRuntimeDescriptorArray(VK_TRUE)
				.setShaderUniformTexelBufferArrayDynamicIndexing(VK_TRUE) // TODO: Make configurable?
				.setShaderStorageTexelBufferArrayDynamicIndexing(VK_TRUE) // TODO: Make configurable?
				.setPNext(&deviceFeatures);

			auto allRequiredDeviceExtensions = get_all_required_device_extensions();
			auto deviceCreateInfo = vk::DeviceCreateInfo()
				.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()))
				.setPQueueCreateInfos(queueCreateInfos.data())
				.setPNext(&descriptorIndexingFeatures) // instead of :setPEnabledFeatures(&deviceFeatures) because we are using vk::PhysicalDeviceFeatures2
				// Whether the device supports these extensions has already been checked during device selection in @ref pick_physical_device
				// TODO: Are these the correct extensions to set here?
				.setEnabledExtensionCount(static_cast<uint32_t>(allRequiredDeviceExtensions.size()))
				.setPpEnabledExtensionNames(allRequiredDeviceExtensions.data())
				.setEnabledLayerCount(static_cast<uint32_t>(supportedValidationLayers.size()))
				.setPpEnabledLayerNames(supportedValidationLayers.data());
			context().mLogicalDevice = context().physical_device().createDevice(deviceCreateInfo);
			// Create a dynamic dispatch loader for extensions
			context().mDynamicDispatch = vk::DispatchLoaderDynamic(context().vulkan_instance(), context().logical_device());

			// Create the queues which have been prepared in the beginning of this method:
			context().mPresentQueue		= device_queue::create(*presentQueue);
			context().mGraphicsQueue	= device_queue::create(*graphicsQueue);
			context().mComputeQueue		= device_queue::create(*computeQueue);
			context().mTransferQueue	= device_queue::create(*transferQueue);

			// Determine distinct queue family indices and distinct (family-id, queue-id)-tuples:
			std::set<uint32_t> uniqueFamilyIndices;
			std::set<std::tuple<uint32_t, uint32_t>> uniqueQueues;
			for (auto q : device_queue::sPreparedQueues) {
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

			context().mContextState = cgb::context_state::fully_initialized;

			return true; // This is just always true
		}, cgb::context_state::halfway_initialized);
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

		pWindow->mImageCreateInfoSwapChain = vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFormat(surfaceFormat.format)
			.setExtent(vk::Extent3D(extent.x, extent.y))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
			.setInitialLayout(vk::ImageLayout::eUndefined);

		auto nope = vk::QueueFlags();
		// See if we can find a queue family which satisfies both criteria: graphics AND presentation (on the given surface)
		auto allInclFamilies = find_queue_families_for_criteria(vk::QueueFlagBits::eGraphics, nope, pWindow->surface());
		if (allInclFamilies.size() != 0) {
			// Found a queue family which supports both!
			// If the graphics queue family and presentation queue family are the same, which will be the case on most hardware, then we should stick to exclusive mode. [2]
			pWindow->mImageCreateInfoSwapChain
				.setSharingMode(vk::SharingMode::eExclusive)
				.setQueueFamilyIndexCount(0) // Optional [2]
				.setPQueueFamilyIndices(nullptr); // Optional [2]
		}
		else {
			auto graphicsFamily = find_queue_families_for_criteria(vk::QueueFlagBits::eGraphics, nope, std::nullopt);
			auto presentFamily = find_queue_families_for_criteria(nope, nope, pWindow->surface());
			assert(graphicsFamily.size() > 0);
			assert(presentFamily.size() > 0);
			pWindow->mQueueFamilyIndices.push_back(std::get<0>(graphicsFamily[0]));
			pWindow->mQueueFamilyIndices.push_back(std::get<0>(presentFamily[0]));
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
		pWindow->mSwapChain = logical_device().createSwapchainKHRUnique(createInfo);
		//pWindow->mSwapChain = logical_device().createSwapchainKHR(createInfo);
		pWindow->mSwapChainImageFormat = surfaceFormat;
		pWindow->mSwapChainExtent = vk::Extent2D(extent.x, extent.y);
		pWindow->mCurrentFrame = 0; // Start af frame 0

		auto swapChainImages = logical_device().getSwapchainImagesKHR(pWindow->swap_chain());
		assert(swapChainImages.size() == pWindow->get_config_number_of_presentable_images());

		// Store the images,
		std::copy(std::begin(swapChainImages), std::end(swapChainImages),
				  std::back_inserter(pWindow->mSwapChainImages));

		// and create one image view per image
		pWindow->mSwapChainImageViews.reserve(pWindow->mSwapChainImages.size());
		for (auto& image : pWindow->mSwapChainImages) {
			// Note:: If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers. You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers. [3]
			pWindow->mSwapChainImageViews.push_back(image_view_t::create(image, pWindow->mImageCreateInfoSwapChain, pWindow->swap_chain_image_format().mFormat));
			pWindow->mSwapChainImageViews.back().enable_shared_ownership();
		}

		// Create a renderpass for the back buffers
		std::vector<attachment> renderpassAttachments = {
			attachment::create_for(pWindow->mSwapChainImageViews[0]),
		};
		auto additionalAttachments = pWindow->get_additional_back_buffer_attachments();
		renderpassAttachments.insert(std::end(renderpassAttachments), std::begin(additionalAttachments), std::end(additionalAttachments)),
		pWindow->mBackBufferRenderpass = renderpass_t::create(renderpassAttachments);
		pWindow->mBackBufferRenderpass.enable_shared_ownership();

		// Create a back buffer per image
		pWindow->mBackBuffers.reserve(pWindow->mSwapChainImageViews.size());
		for (auto& imView: pWindow->mSwapChainImageViews) {
			auto imExtent = imView->image_config().extent;

			// Create one image view per attachment
			std::vector<image_view> imageViews;
			imageViews.reserve(renderpassAttachments.size());
			imageViews.push_back(imView); // The color attachment is added in any case
			for (auto& aa : additionalAttachments) {
				if (is_depth_format(aa.format())) {
					// TODO: can setting the config-alteration function for depth attachments be somehow abstracted?! e.g. by moving it into the framebuffer class (or a framebuffer's ::create method)
					auto depthView = image_view_t::create(image_t::create(imExtent.width, imExtent.height, aa.format(), false, 1, cgb::memory_usage::device, cgb::image_usage::read_only_depth_stencil_attachment,
						[](image_t& imageToConfig) { imageToConfig.config().setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment); })); 
					// TODO: Disable shared ownership, once the noexcept-hell has been resolved
					depthView.enable_shared_ownership();
					imageViews.push_back(std::move(depthView));
				}
				else {
					imageViews.emplace_back(image_view_t::create(image_t::create(imExtent.width, imExtent.height, aa.format(), false, 1, memory_usage::device, cgb::image_usage::versatile_color_attachment)))
						.enable_shared_ownership(); // TODO: Disable shared ownership, once the noexcept-hell has been resolved
				}
			}

			pWindow->mBackBuffers.push_back(framebuffer_t::create(pWindow->mBackBufferRenderpass, std::move(imageViews), imExtent.width, imExtent.height));
		}

		// ============= SYNCHRONIZATION OBJECTS ===========
		// Create them here, already.
		auto numSyncObjects = pWindow->get_config_number_of_concurrent_frames();
		pWindow->mFences.reserve(numSyncObjects);
		pWindow->mImageAvailableSemaphores.reserve(numSyncObjects);
		pWindow->mRenderFinishedSemaphores.reserve(numSyncObjects);
		for (uint32_t i = 0; i < numSyncObjects; ++i) {
			pWindow->mFences.push_back(fence_t::create(true)); // true => Create the fences in signalled state, so that `cgb::context().logical_device().waitForFences` at the beginning of `window::render_frame` is not blocking forever, but can continue immediately.
			pWindow->mImageAvailableSemaphores.push_back(semaphore_t::create());
			pWindow->mRenderFinishedSemaphores.push_back(semaphore_t::create());
		}

	}

	//pipeline vulkan::create_ray_tracing_pipeline(
	//	const std::vector<std::tuple<shader_type, shader*>>& pShaderInfos,
	//	const std::vector<vk::DescriptorSetLayout>& pDescriptorSetLayouts)
	//{
	//	// CREATE PIPELINE
	//	auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo()
	//		.setSetLayoutCount(static_cast<uint32_t>(pDescriptorSetLayouts.size()))
	//		.setPSetLayouts(pDescriptorSetLayouts.data())
	//		.setPushConstantRangeCount(0u)
	//		.setPPushConstantRanges(nullptr);
	//	auto pipelineLayout = mLogicalDevice.createPipelineLayout(pipelineLayoutInfo);

	//	// Gather the shader infos
	//	std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
	//	//std::transform(std::begin(pShaderInfos), std::end(pShaderInfos),
	//	//			   std::back_inserter(shaderStages),
	//	//			   [](const auto& tpl) {
	//	//				   return vk::PipelineShaderStageCreateInfo()
	//	//					   .setStage(convert(std::get<shader_type>(tpl)))
	//	//					   .setModule(std::get<shader*>(tpl)->handle())
	//	//					   .setPName("main"); // TODO: support different entry points?!
	//	//			   });

	//	// Create shader groups
	//	std::array shaderGroups = {
	//		// group0 = [raygen]
	//		vk::RayTracingShaderGroupCreateInfoNV()
	//		.setType(vk::RayTracingShaderGroupTypeNV::eGeneral)
	//		.setGeneralShader(0) // Just the ray generation shader here, nothing special
	//		.setClosestHitShader(VK_SHADER_UNUSED_NV)
	//		.setAnyHitShader(VK_SHADER_UNUSED_NV)
	//		.setIntersectionShader(VK_SHADER_UNUSED_NV)
	//		,
	//		// group1 = [chit]
	//		vk::RayTracingShaderGroupCreateInfoNV()
	//		.setType(vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup)
	//		.setGeneralShader(VK_SHADER_UNUSED_NV) // Unused because we're using the stock triangles intersection functionality
	//		.setClosestHitShader(1u) // If we get closest hits, we'd like to have our 2nd entry to the shader table executed
	//		.setAnyHitShader(VK_SHADER_UNUSED_NV)
	//		.setIntersectionShader(VK_SHADER_UNUSED_NV)
	//		,
	//		// group2 = [chit1]
	//		vk::RayTracingShaderGroupCreateInfoNV()
	//		.setType(vk::RayTracingShaderGroupTypeNV::eTrianglesHitGroup)
	//		.setGeneralShader(VK_SHADER_UNUSED_NV) 
	//		.setClosestHitShader(2u) 
	//		.setAnyHitShader(VK_SHADER_UNUSED_NV)
	//		.setIntersectionShader(VK_SHADER_UNUSED_NV)
	//		,
	//		// group3 = [miss0]
	//		vk::RayTracingShaderGroupCreateInfoNV()
	//		.setType(vk::RayTracingShaderGroupTypeNV::eGeneral)
	//		.setGeneralShader(3u) // Ein Miss Shader hat mit einem bestimmten Hit nix zu tun, deswegen is der nicht in der Hit Group mit dabei, sondern ein separater Eintrag
	//		.setClosestHitShader(VK_SHADER_UNUSED_NV)
	//		.setAnyHitShader(VK_SHADER_UNUSED_NV)
	//		.setIntersectionShader(VK_SHADER_UNUSED_NV)
	//		,
	//		// group4 = [miss1]
	//		vk::RayTracingShaderGroupCreateInfoNV()
	//		.setType(vk::RayTracingShaderGroupTypeNV::eGeneral)
	//		.setGeneralShader(4u) // Ein Miss Shader hat mit einem bestimmten Hit nix zu tun, deswegen is der nicht in der Hit Group mit dabei, sondern ein separater Eintrag
	//		.setClosestHitShader(VK_SHADER_UNUSED_NV)
	//		.setAnyHitShader(VK_SHADER_UNUSED_NV)
	//		.setIntersectionShader(VK_SHADER_UNUSED_NV)
	//	};

	//	// Do it:
	//	auto pipelineCreateInfo = vk::RayTracingPipelineCreateInfoNV()
	//		.setStageCount(static_cast<uint32_t>(shaderStages.size()))
	//		.setPStages(shaderStages.data())
	//		.setGroupCount(static_cast<uint32_t>(shaderGroups.size()))
	//		.setPGroups(shaderGroups.data())
	//		.setMaxRecursionDepth(2u) // Ho ho ho! Wir recursen!
	//		.setLayout(pipelineLayout);

	//	throw std::exception("wip");
	//	//return pipeline(
	//	//	pipelineLayout,
	//	//	context().logical_device().createRayTracingPipelineNV(
	//	//		nullptr,						// no pipeline cache
	//	//		pipelineCreateInfo,				// pipeline description
	//	//		nullptr,						// no allocation callbacks
	//	//		context().dynamic_dispatch()));	// dynamic dispatch for extension
	//}

	//std::vector<framebuffer> vulkan::create_framebuffers(const vk::RenderPass& renderPass, window* pWindow, const image_view_t& pDepthImageView)
	//{
	//	std::vector<framebuffer> framebuffers;
	//	auto extent = pWindow->swap_chain_extent();
	//	for (const auto& imageView : pWindow->swap_chain_image_views()) {
	//		std::array attachments = { imageView.get(), pDepthImageView.view_handle() };
	//		auto framebufferInfo = vk::FramebufferCreateInfo()
	//			.setRenderPass(renderPass)
	//			.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
	//			.setPAttachments(attachments.data())
	//			.setWidth(extent.width)
	//			.setHeight(extent.height)
	//			.setLayers(1u); // number of layers in image arrays [6]

	//		framebuffers.push_back(framebuffer{ mLogicalDevice.createFramebuffer( framebufferInfo ) });
	//	}
	//	return framebuffers;
	//}

	command_pool& vulkan::get_command_pool_for_queue_family(uint32_t pQueueFamilyIndex)
	{
		auto it = std::find_if(std::begin(mCommandPools), std::end(mCommandPools),
							   [family_idx = pQueueFamilyIndex, thread_id = std::this_thread::get_id()](const std::tuple<std::thread::id, command_pool>& existing) {
								   return std::get<0>(existing) == thread_id && std::get<1>(existing).queue_family_index() == family_idx;
							   });
		if (it == std::end(mCommandPools)) {
			// TODO: Do we want different parameters depending on the queue? 
			//       Like, for instance re-usable buffers for the graphics queue?
			//		 There is a parameter to `command_pool::create` where this could be specified!
			return std::get<1>(mCommandPools.emplace_back(std::this_thread::get_id(), command_pool::create(pQueueFamilyIndex)));
		}
		return std::get<1>(*it);
	}

	command_pool& vulkan::get_command_pool_for_queue(const device_queue& pQueue)
	{
		return get_command_pool_for_queue_family(pQueue.family_index());
	}

	uint32_t vulkan::find_memory_type_index(uint32_t pMemoryTypeBits, vk::MemoryPropertyFlags pMemoryProperties)
	{
		// The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. 
		// Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for 
		// when VRAM runs out. The different types of memory exist within these heaps. Right now we'll 
		// only concern ourselves with the type of memory and not the heap it comes from, but you can 
		// imagine that this can affect performance.
		auto memProperties = physical_device().getMemoryProperties();
		for (auto i = 0u; i < memProperties.memoryTypeCount; ++i) {
			if ((pMemoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & pMemoryProperties) == pMemoryProperties) {
				return i;
			}
		}
		throw std::runtime_error("failed to find suitable memory type!");
	}

	std::shared_ptr<descriptor_pool> vulkan::get_descriptor_pool_for_layouts(std::initializer_list<std::reference_wrapper<descriptor_set_layout>> pLayouts)
	{
		// We'll allocate the pools per thread
		auto threadId = std::this_thread::get_id();
		auto& pools = mDescriptorPools[threadId];

		// Compute the requirements:
		auto alloc_requirements = descriptor_alloc_request::create(std::move(pLayouts));
		// First of all, do some cleanup => remove all pools which no longer exist:
		pools.erase(std::remove_if(std::begin(pools), std::end(pools), [](const std::weak_ptr<descriptor_pool>& ptr) {
			return ptr.expired();
		}), std::end(pools));

		// Find a pool which is capable of allocating this:
		for (auto& pool : pools) {
			if (auto sptr = pool.lock()) {
				if (sptr->has_capacity_for(alloc_requirements)) {
					return sptr;
				}
			}
		}

		// We weren't lucky => create a new pool:
		auto newPool = descriptor_pool::create(alloc_requirements.accumulated_pool_sizes(), settings::gDescriptorPoolSizeFactor);
		pools.emplace_back(newPool); // Store as a weak_ptr
		return newPool;
	}

	vk::DescriptorPool vulkan::get_descriptor_pool()
	{
		if (!mWorkaroundDescriptorPool) {

			std::array poolSizes = {
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eUniformBuffer)
					.setDescriptorCount(1024u) // TODO: is that a good pool size? and what to do beyond that number?
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eCombinedImageSampler)
					.setDescriptorCount(1024u) // TODO: is that a good pool size? and what to do beyond that number?
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eStorageImage)
					.setDescriptorCount(1024u) // TODO: is that a good pool size? and what to do beyond that number?
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eAccelerationStructureNV)
					.setDescriptorCount(1024u) // TODO: is that a good pool size? and what to do beyond that number?
				,
				// Added because of IMGUI:
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eSampler)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eSampledImage)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eUniformTexelBuffer)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eStorageTexelBuffer)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eStorageBuffer)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eUniformBufferDynamic)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eStorageBufferDynamic)
					.setDescriptorCount(1024u)
				,
				vk::DescriptorPoolSize()
					.setType(vk::DescriptorType::eInputAttachment)
					.setDescriptorCount(1024u)
			};

			auto poolInfo = vk::DescriptorPoolCreateInfo()
				.setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
				.setPPoolSizes(poolSizes.data())
				.setMaxSets(128u) // TODO: is that a good max sets-number? and what to do beyond that number?
				.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet); // The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not: VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We're not going to touch the descriptor set after creating it, so we don't need this flag. [10]
			// TODO: WTF, why do we need the eFreeDescriptorSet flag ^ here? What does it mean?
			mWorkaroundDescriptorPool = logical_device().createDescriptorPoolUnique(poolInfo);
		}
		return mWorkaroundDescriptorPool.get();

			//descriptor_pool result;
			//return result;
	}

	std::vector<vk::UniqueDescriptorSet> vulkan::create_descriptor_set(std::vector<vk::DescriptorSetLayout> pData)
	{

		auto allocInfo = vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(get_descriptor_pool()) // TODO: set a valid descriptor pool
			.setDescriptorSetCount(static_cast<uint32_t>(pData.size()))
			.setPSetLayouts(pData.data());
		auto descriptorSets = logical_device().allocateDescriptorSetsUnique(allocInfo); // The call to vkAllocateDescriptorSets will allocate descriptor sets, each with one uniform buffer descriptor. [10]
		std::vector<vk::UniqueDescriptorSet> result;
		std::transform(std::begin(descriptorSets), std::end(descriptorSets),
					   std::back_inserter(result),
					   [](auto& vkDescSet) {
						   return std::move(vkDescSet);
					   });
		return result;
	}

	bool vulkan::is_format_supported(vk::Format pFormat, vk::ImageTiling pTiling, vk::FormatFeatureFlags pFormatFeatures)
	{
		auto formatProps = mPhysicalDevice.getFormatProperties(pFormat);
		if (pTiling == vk::ImageTiling::eLinear 
			&& (formatProps.linearTilingFeatures & pFormatFeatures) == pFormatFeatures) {
			return true;
		}
		else if (pTiling == vk::ImageTiling::eOptimal 
				 && (formatProps.optimalTilingFeatures & pFormatFeatures) == pFormatFeatures) {
			return true;
		} 
		return false;
	}


	vk::PhysicalDeviceRayTracingPropertiesNV vulkan::get_ray_tracing_properties()
	{
		vk::PhysicalDeviceRayTracingPropertiesNV rtProps;
		vk::PhysicalDeviceProperties2 props2;
		props2.pNext = &rtProps;
		mPhysicalDevice.getProperties2(&props2);
		return rtProps;
	}


	// REFERENCES:
	// [1] Vulkan Tutorial, Logical device and queues, https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Logical_device_and_queues
	// [2] Vulkan Tutorial, Swap chain, https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain
	// [3] Vulkan Tutorial, Image views, https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Image_views
	// [4] Vulkan Tutorial, Fixed functions, https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
	// [5] Vulkan Tutorial, Render passes, https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
	// [6] Vulkan Tutorial, Framebuffers, https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Framebuffers
	// [7] Vulkan Tutorial, Command Buffers, https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Command_buffers
	// [8] Vulkan Tutorial, Rendering and presentation, https://vulkan-tutorial.com/Drawing_a_triangle/Drawing/Rendering_and_presentation
	// [9] Vulkan Tutorial, Vertex buffers, https://vulkan-tutorial.com/Vertex_buffers/Vertex_buffer_creation
	// [10] Vulkan Tutorial, Descriptor pool and sets, https://vulkan-tutorial.com/Uniform_buffers/Descriptor_pool_and_sets
}
