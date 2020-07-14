#pragma once
#include <exekutor.hpp>

namespace xk
{
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w)
	{ }

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, physical_device_selection_hint& aValue, Args&... args)
	{
		s.mPhysicalDeviceSelectionHint = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, application_name& aValue, Args&... args)
	{
		s.mApplicationName = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, application_version& aValue, Args&... args)
	{
		s.mApplicationVersion = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, required_instance_extensions& aValue, Args&... args)
	{
		s.mRequiredInstanceExtensions = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, validation_layers& aValue, Args&... args)
	{
		s.mValidationLayers = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, required_device_extensions& aValue, Args&... args)
	{
		s.mRequiredDeviceExtensions = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, timer_interface& aValue, Args&... args)
	{
		t = &aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, timer_interface* aValue, Args&... args)
	{
		t = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, invoker_interface& aValue, Args&... args)
	{
		i = &aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, invoker_interface* aValue, Args&... args)
	{
		i = aValue;
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, window* aValue, Args&... args)
	{
		w.push_back(aValue);
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, cg_element& aValue, Args&... args)
	{
		e.push_back(&aValue);
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, vk::PhysicalDeviceFeatures& phdf, vk::PhysicalDeviceVulkan12Features& v12f, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, cg_element* aValue, Args&... args)
	{
		e.push_back(aValue);
		add_config(s, phdf, v12f, t, i, e, w, args...);
	}
	
	template <typename... Args>
	static void execute(Args&&... args)
	{
		varying_update_timer defaultTimer;
		sequential_executor defaultInvoker;
		
		settings s{};
		
		vk::PhysicalDeviceFeatures phdf = vk::PhysicalDeviceFeatures()
				.setGeometryShader(VK_TRUE)
				.setTessellationShader(VK_TRUE)
				.setSamplerAnisotropy(VK_TRUE)
				.setVertexPipelineStoresAndAtomics(VK_TRUE)
				.setFragmentStoresAndAtomics(VK_TRUE)
				.setShaderStorageImageExtendedFormats(VK_TRUE)
				.setSampleRateShading(VK_TRUE);

		vk::PhysicalDeviceVulkan12Features v12f = vk::PhysicalDeviceVulkan12Features()
				.setDescriptorBindingVariableDescriptorCount(VK_TRUE)
				.setRuntimeDescriptorArray(VK_TRUE)
				.setShaderUniformTexelBufferArrayDynamicIndexing(VK_TRUE)
				.setShaderStorageTexelBufferArrayDynamicIndexing(VK_TRUE)
				.setDescriptorIndexing(VK_TRUE)
				.setBufferDeviceAddress(VK_TRUE);
		
		timer_interface* t = &defaultTimer;
		invoker_interface* i = &defaultInvoker;
		std::vector<cg_element*> e;
		std::vector<window*> w;
		add_config(s, phdf, v12f, t, i, e, w, args...);

		context().initialize(s, phdf, v12f);
		{
			composition c(t, i, w, e);
			c.start();
		}
		// Context goes out of scope later, all good
	}
	
	
}
