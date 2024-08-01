#pragma once

#include "composition.hpp"
#include <vector>

namespace avk
{
#if VK_HEADER_VERSION >= 239
#define CONFIG_STRUCTS_DECLARATIONS vk::PhysicalDeviceAccelerationStructureFeaturesKHR& acsFtrs, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& rtpFtrs, vk::PhysicalDeviceRayQueryFeaturesKHR& rquFtrs, vk::PhysicalDeviceMeshShaderFeaturesEXT& meShFtr
#define CONFIG_PARAMETERS_PASSED_ON acsFtrs, rtpFtrs, rquFtrs, meShFtr
#elif VK_HEADER_VERSION >= 162
#define CONFIG_STRUCTS_DECLARATIONS vk::PhysicalDeviceAccelerationStructureFeaturesKHR& acsFtrs, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& rtpFtrs, vk::PhysicalDeviceRayQueryFeaturesKHR& rquFtrs
#define CONFIG_PARAMETERS_PASSED_ON acsFtrs, rtpFtrs, rquFtrs
#else
#define CONFIG_STRUCTS_DECLARATIONS vk::PhysicalDeviceRayTracingFeaturesKHR& rtf
#define CONFIG_PARAMETERS_PASSED_ON rtf
#endif
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w)
	{ }

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, physical_device_selection_hint& aValue, Args&... args)
	{
		s.mPhysicalDeviceSelectionHint = aValue;
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, application_name& aValue, Args&... args)
	{
		s.mApplicationName = aValue;
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, application_version& aValue, Args&... args)
	{
		s.mApplicationVersion = aValue;
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, required_instance_extensions& aValue, Args&... args)
	{
		s.mRequiredInstanceExtensions.mExtensions.insert(std::end(s.mRequiredInstanceExtensions.mExtensions), std::begin(aValue.mExtensions), std::end(aValue.mExtensions));
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, required_device_extensions& aValue, Args&... args)
	{
		s.mRequiredDeviceExtensions.mExtensions.insert(std::end(s.mRequiredDeviceExtensions.mExtensions), std::begin(aValue.mExtensions), std::end(aValue.mExtensions));
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, optional_device_extensions& aValue, Args&... args)
	{
		s.mOptionalDeviceExtensions.mExtensions.insert(std::end(s.mOptionalDeviceExtensions.mExtensions), std::begin(aValue.mExtensions), std::end(aValue.mExtensions));
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, window* aValue, Args&... args)
	{
		w.push_back(aValue);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, invokee& aValue, Args&... args)
	{
		e.push_back(&aValue);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, invokee* aValue, Args&... args)
	{
		e.push_back(aValue);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(validation_layers&)> fu, Args&... args)
	{
		fu(s.mValidationLayers);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceFeatures&)> fu, Args&... args)
	{
		fu(phdf);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceVulkan11Features&)> fu, Args&... args)
	{
		fu(v11f);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceVulkan12Features&)> fu, Args&... args)
	{
		fu(v12f);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

#if VK_HEADER_VERSION >= 162
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceAccelerationStructureFeaturesKHR&)> fu, Args&... args)
	{
		fu(acsFtrs);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR&)> fu, Args&... args)
	{
		fu(rtpFtrs);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayQueryFeaturesKHR&)> fu, Args&... args)
	{
		fu(rquFtrs);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
#else
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayTracingFeaturesKHR&)> fu, Args&... args)
	{
		fu(rtf);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
#endif

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::DebugUtilsMessageSeverityFlagsEXT&)> fu, Args&... args)
	{
		fu(s.mEnabledDebugUtilsMessageSeverities);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::DebugUtilsMessageTypeFlagsEXT&)> fu, Args&... args)
	{
		fu(s.mEnabledDebugUtilsMessageTypes);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}

#if VK_HEADER_VERSION >= 239
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceMeshShaderFeaturesEXT&)> fu, Args&... args)
	{
		fu(meShFtr);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
#endif

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan11Features& v11f, vk::PhysicalDeviceVulkan12Features& v12f, CONFIG_STRUCTS_DECLARATIONS, std::vector<invokee*>& e, std::vector<window*>& w, physical_device_features_pNext_chain_entry& aValue, Args&... args)
	{
		s.mPhysicalDeviceFeaturesPNextChainEntries.push_back(aValue);
		add_config(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON, e, w, args...);
	}
	
	/**	>>>>>>>>>>>>>> Start the Gears <<<<<<<<<<<<<<<
	 *
	 *	You may pass the following configuration parameters:
	 *	- physical_device_selection_hint&								              ... To declare which physical device shall be used.
	 *	- application_name&												              ... To declare the name of this application
	 *	- application_version&											              ... To declare the application version
	 *	- required_instance_extensions&									              ... A struct to configure required instance extensions which must be supported by the Vulkan instance and shall be activated.
	 *	- required_device_extensions&									              ... A struct to configure required device extensions which must be supported by the device.
	 *	- physical_device_features_pNext_chain_entry                                  ... A struct that contains a pNext pointer to a configuration struct which is to be added to the vk::PhysicalDeviceFeatures2 pNext chain.
	 *	- window*														              ... A window that shall be usable during the runtime of start().
	 *	- invokee& or invokee*											              ... Pointer or reference to an invokee which outlives the runtime of start().
	 *	- std::function<void(validation_layers&)						              ... A function which can be used to modify the struct containing config for validation layers and validation layer features
	 *	- std::function<void(vk::PhysicalDeviceFeatures&)>				              ... A function which can be used to modify the vk::PhysicalDeviceFeatures. Modify the values of the passed vk::PhysicalDeviceFeatues directly!
	 *	- std::function<void(vk::PhysicalDeviceVulkan11Features&)>		              ... A function which can be used to modify the vk::PhysicalDeviceVulkan11Features. Modify the values of the passed vk::PhysicalDeviceVulkan11Features directly!
	 *	- std::function<void(vk::PhysicalDeviceVulkan12Features&)>		              ... A function which can be used to modify the vk::PhysicalDeviceVulkan12Features. Modify the values of the passed vk::PhysicalDeviceVulkan12Features directly!
	 *	- std::function<void(vk::PhysicalDeviceAccelerationStructureFeaturesKHR&)>	  ... A function which can be used to modify the vk::PhysicalDeviceRayTracingFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayTracingFeaturesKHR directly!
	 *	- std::function<void(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR&)>	      ... A function which can be used to modify the vk::PhysicalDeviceRayTracingPipelineFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayTracingPipelineFeaturesKHR directly!
	 *	- std::function<void(vk::PhysicalDeviceRayQueryFeaturesKHR&)>	              ... A function which can be used to modify the vk::PhysicalDeviceRayQueryFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayQueryFeaturesKHR directly!
	 *	- std::function<void(vk::DebugUtilsMessageSeverityFlagsEXT&)>                 ... A function which can be used to modify the vk::DebugUtilsMessageSeverityFlagsEXT flags which will be enabled for the debug utils.
	 *	- std::function<void(vk::DebugUtilsMessageTypeFlagsEXT&)>                     ... A function which can be used to modify the vk::DebugUtilsMessageTypeFlagsEXT flags which will be enabled for the debug utils.
	 *	- std::function<void(vk::PhysicalDeviceMeshShaderFeaturesEXT&)>               ... A function which can be used to modify the vk::PhysicalDeviceMeshShaderFeaturesEXT. Modify the values of the passed vk::PhysicalDeviceMeshShaderFeaturesEXT directly!
	 */
	template <typename... Args>
	static avk::composition configure_and_compose(Args&&... args)
	{
		settings s{};

		vk::PhysicalDeviceFeatures phdf = vk::PhysicalDeviceFeatures{}
			.setGeometryShader(VK_TRUE)
			.setTessellationShader(VK_TRUE)
			.setSamplerAnisotropy(VK_TRUE)
			.setVertexPipelineStoresAndAtomics(VK_TRUE)
			.setFragmentStoresAndAtomics(VK_TRUE)
			.setShaderStorageImageExtendedFormats(VK_TRUE)
			.setSampleRateShading(VK_TRUE)
			.setFillModeNonSolid(VK_TRUE);

		vk::PhysicalDeviceVulkan11Features v11f = vk::PhysicalDeviceVulkan11Features{};

		vk::PhysicalDeviceVulkan12Features v12f = vk::PhysicalDeviceVulkan12Features{}
			.setDescriptorBindingVariableDescriptorCount(VK_TRUE)
			.setRuntimeDescriptorArray(VK_TRUE)
			.setShaderUniformTexelBufferArrayDynamicIndexing(VK_TRUE)
			.setShaderStorageTexelBufferArrayDynamicIndexing(VK_TRUE)
			.setDescriptorIndexing(VK_TRUE)
			.setBufferDeviceAddress(VK_FALSE);

#if VK_HEADER_VERSION >= 162
		auto acsFtrs = vk::PhysicalDeviceAccelerationStructureFeaturesKHR{}.setAccelerationStructure(VK_FALSE);
		auto rtpFtrs = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR{}.setRayTracingPipeline(VK_FALSE);
		auto rquFtrs = vk::PhysicalDeviceRayQueryFeaturesKHR{}.setRayQuery(VK_FALSE);
#else
		vk::PhysicalDeviceRayTracingFeaturesKHR rtf = vk::PhysicalDeviceRayTracingFeaturesKHR{}
		.setRayTracing(VK_FALSE);
#endif
#if VK_HEADER_VERSION >= 239
		auto meShFtr = vk::PhysicalDeviceMeshShaderFeaturesEXT{}; // all set to false
#endif

		std::vector<invokee*> e;
		std::vector<window*> w;
		add_config(s, phdf, v11f, v12f,
			CONFIG_PARAMETERS_PASSED_ON,
			e, w, args...
		);

		context().initialize(s, phdf, v11f, v12f, CONFIG_PARAMETERS_PASSED_ON);
		return avk::composition{ w, e };
		// Context goes out of scope later, all good
	}
}

