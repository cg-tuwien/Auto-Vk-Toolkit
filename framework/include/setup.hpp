#pragma once
#include <gvk.hpp>

namespace gvk
{
#if VK_HEADER_VERSION >= 162
#define RAY_TRACING_CONFIG_PARAMETERS vk::PhysicalDeviceAccelerationStructureFeaturesKHR& acsFtrs, vk::PhysicalDeviceRayTracingPipelineFeaturesKHR& rtpFtrs, vk::PhysicalDeviceRayQueryFeaturesKHR& rquFtrs
#define RAY_TRACING_PASS_ON_PARAMETERS acsFtrs, rtpFtrs, rquFtrs
#else
#define RAY_TRACING_CONFIG_PARAMETERS vk::PhysicalDeviceRayTracingFeaturesKHR& rtf
#define RAY_TRACING_PASS_ON_PARAMETERS rtf
#endif
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w)
	{ }

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, physical_device_selection_hint& aValue, Args&... args)
	{
		s.mPhysicalDeviceSelectionHint = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, application_name& aValue, Args&... args)
	{
		s.mApplicationName = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, application_version& aValue, Args&... args)
	{
		s.mApplicationVersion = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, required_instance_extensions& aValue, Args&... args)
	{
		s.mRequiredInstanceExtensions = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, required_device_extensions& aValue, Args&... args)
	{
		s.mRequiredDeviceExtensions = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, timer_interface& aValue, Args&... args)
	{
		t = &aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, timer_interface* aValue, Args&... args)
	{
		t = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, invoker_interface& aValue, Args&... args)
	{
		i = &aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, invoker_interface* aValue, Args&... args)
	{
		i = aValue;
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, window* aValue, Args&... args)
	{
		w.push_back(aValue);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, invokee& aValue, Args&... args)
	{
		e.push_back(&aValue);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, invokee* aValue, Args&... args)
	{
		e.push_back(aValue);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(validation_layers&)> fu, Args&... args)
	{
		fu(s.mValidationLayers);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceFeatures&)> fu, Args&... args)
	{
		fu(phdf);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceVulkan12Features&)> fu, Args&... args)
	{
		fu(v12f);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
	
#if VK_HEADER_VERSION >= 162
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceAccelerationStructureFeaturesKHR&)> fu, Args&... args)
	{
		fu(acsFtrs);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR&)> fu, Args&... args)
	{
		fu(rtpFtrs);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayQueryFeaturesKHR&)> fu, Args&... args)
	{
		fu(rquFtrs);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
#else
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, RAY_TRACING_CONFIG_PARAMETERS, timer_interface*& t, invoker_interface*& i, std::vector<invokee*>& e, std::vector<window*>& w, std::function<void(vk::PhysicalDeviceRayTracingFeaturesKHR&)> fu, Args&... args)
	{
		fu(rtf);
		add_config(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS, t, i, e, w, args...);
	}
#endif

	/**	>>>>>>>>>>>>>> Start the Gears <<<<<<<<<<<<<<<
	 *
	 *	You may pass the following configuration parameters:
	 *	- physical_device_selection_hint&								              ... To declare which physical device shall be used.
	 *	- application_name&												              ... To declare the name of this application
	 *	- application_version&											              ... To declare the application version
	 *	- required_instance_extensions&									              ... A struct to configure required instance extensions which must be supported by the Vulkan instance and shall be activated.
	 *	- required_device_extensions&									              ... A struct to configure required device extensions which must be supported by the device.
	 *	- timer_interface& or timer_interface*							              ... Pointer or reference to timer class which handles gvk::time(). The timer must outlive the runtime of start().
	 *	- invoker_interface& or invoker_interface*						              ... Pointer or reference to an invoker which invokes all the invokee's members. The invoker must outlive the runtime of start().
	 *	- window*														              ... A window that shall be usable during the runtime of start().
	 *	- invokee& or invokee*											              ... Pointer or reference to an invokee which outlives the runtime of start().
	 *	- std::function<void(validation_layers&)						              ... A function which can be used to modify the struct containing config for validation layers and validation layer features
	 *	- std::function<void(vk::PhysicalDeviceFeatures&)>				              ... A function which can be used to modify the vk::PhysicalDeviceFeatures. Modify the values of the passed vk::PhysicalDeviceFeatues directly!
	 *	- std::function<void(vk::PhysicalDeviceVulkan12Features&)>		              ... A function which can be used to modify the vk::PhysicalDeviceVulkan12Features. Modify the values of the passed vk::PhysicalDeviceVulkan12Features directly!
	 *	- std::function<void(vk::PhysicalDeviceAccelerationStructureFeaturesKHR&)>	  ... A function which can be used to modify the vk::PhysicalDeviceRayTracingFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayTracingFeaturesKHR directly!
	 *	- std::function<void(vk::PhysicalDeviceRayTracingPipelineFeaturesKHR&)>	      ... A function which can be used to modify the vk::PhysicalDeviceRayTracingPipelineFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayTracingPipelineFeaturesKHR directly!
	 *	- std::function<void(vk::PhysicalDeviceRayQueryFeaturesKHR&)>	              ... A function which can be used to modify the vk::PhysicalDeviceRayQueryFeaturesKHR. Modify the values of the passed vk::PhysicalDeviceRayQueryFeaturesKHR directly!
	 */
	template <typename... Args>
	static void start(Args&&... args)
	{
		varying_update_timer defaultTimer;
		sequential_invoker defaultInvoker;
		
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
		
		timer_interface* t = &defaultTimer;
		invoker_interface* i = &defaultInvoker;
		std::vector<invokee*> e;
		std::vector<window*> w;
		add_config(s, phdf, v12f, 
#if VK_HEADER_VERSION >= 162
			acsFtrs, rtpFtrs, rquFtrs,
#else
			rtf,
#endif
			t, i, e, w, args...);

		context().initialize(s, phdf, v12f, RAY_TRACING_PASS_ON_PARAMETERS);
		{
			composition c(t, i, w, e);
			c.start();
		}
		// Context goes out of scope later, all good
	}
	
	
}
