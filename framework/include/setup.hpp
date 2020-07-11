#pragma once
#include <exekutor.hpp>

namespace xk
{
	/** Set a hint about which device to select. This could be, e.g., "Intel" or "RTX". */
	struct physical_device_selection_hint
	{
		std::string mValue = "";
	};

	/** Set this to your application's name */
	struct application_name
	{
		std::string mValue = "Exekutor Application";
	};

	/** Set this to your application's version */
	struct application_version
	{
		uint32_t mValue = ak::make_version(1u, 0u, 0u);
	};

	/** Fill this vector with further required instance extensions, if required */
	struct required_instance_extensions
	{
		required_instance_extensions(const char* aExtensionName = nullptr)
		{
			if (nullptr != aExtensionName) {
				mExtensions.push_back(aExtensionName);
			}
		}

		required_instance_extensions& add_extension(const char* aExtensionName)
		{
			assert(nullptr != aExtensionName);
			mExtensions.push_back(aExtensionName);
			return *this;
		}

		std::vector<const char*> mExtensions;
	};

	/** Modify this vector or strings according to your needs. 
	 *  By default, it will be initialized with a default validation layer name.
	 */
	struct validation_layers
	{
		validation_layers(const char* aLayerName = nullptr) : mEnableInRelease{ false }
		{
			mLayers.push_back("VK_LAYER_KHRONOS_validation");
			if (nullptr != aLayerName) {
				mLayers.push_back(aLayerName);
			}
		}

		validation_layers& add_extension(const char* aExtensionName)
		{
			assert(nullptr != aExtensionName);
			mLayers.push_back(aExtensionName);
			return *this;
		}

		/** Set to true to also enable validation layers in RELEASE and PUBLISH builds. Default = false. */
		validation_layers& enable_in_release_mode(bool aEnable)
		{
			mEnableInRelease = aEnable;
			return *this;
		}

		std::vector<const char*> mLayers;
		bool mEnableInRelease;
	};

	/** Fill this vector with required device extensions, if required */
	struct required_device_extensions
	{
		required_device_extensions(const char* aExtensionName = nullptr)
		{
			if (nullptr != aExtensionName) {
				mExtensions.push_back(aExtensionName);
			}
		}

		required_device_extensions& add_extension(const char* aExtensionName)
		{
			assert(nullptr != aExtensionName);
			mExtensions.push_back(aExtensionName);
			return *this;
		}

		std::vector<const char*> mExtensions;
	};

	struct enable_features
	{
		/** Enables/disables the buffer device address feature.
		 *  Default = false.
		 */
		enable_features& enable_buffer_device_address(bool aEnable)
		{
			mEnableBufferDeviceAddress = aEnable;
			return *this;
		}
		
		bool mEnableBufferDeviceAddress = false;
	};

	struct settings
	{
		physical_device_selection_hint mPhysicalDeviceSelectionHint;
		application_name mApplicationName;
		application_version mApplicationVersion;
		required_instance_extensions mRequiredInstanceExtensions;
		validation_layers mValidationLayers;
		required_device_extensions mRequiredDeviceExtensions;
		enable_features mEnabledFeatures;
	};

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<window*>& w, std::vector<cg_element*>& e)
	{ }

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, physical_device_selection_hint& aValue, Args&... args)
	{
		s.mPhysicalDeviceSelectionHint = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, application_name& aValue, Args&... args)
	{
		s.mApplicationName = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, application_version& aValue, Args&... args)
	{
		s.mApplicationVersion = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, required_instance_extensions& aValue, Args&... args)
	{
		s.mRequiredInstanceExtensions = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, validation_layers& aValue, Args&... args)
	{
		s.mValidationLayers = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, required_device_extensions& aValue, Args&... args)
	{
		s.mRequiredDeviceExtensions = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, enable_features& aValue, Args&... args)
	{
		s.mEnabledFeatures = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, timer_interface& aValue, Args&... args)
	{
		t = &aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, timer_interface* aValue, Args&... args)
	{
		t = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, invoker_interface& aValue, Args&... args)
	{
		i = &aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, invoker_interface* aValue, Args&... args)
	{
		i = aValue;
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, cg_element& aValue, Args&... args)
	{
		e.push_back(&aValue);
		add_config(s, t, i, e, w, args...);
	}

	template <typename... Args>
	static void add_config(settings& s, timer_interface*& t, invoker_interface*& i, std::vector<cg_element*>& e, std::vector<window*>& w, cg_element* aValue, Args&... args)
	{
		e.push_back(aValue);
		add_config(s, t, i, e, w, args...);
	}

	
	template <typename... Args>
	static void execute(Args&... args)
	{
		varying_update_timer defaultTimer;
		sequential_executor defaultInvoker;
		
		settings s{};
		timer_interface* t = &defaultTimer;
		invoker_interface* i = &defaultInvoker;
		std::vector<cg_element*> e;
		std::vector<window*> w;
		add_config(s, t, i, e, w, args...);

		// TODO: Create context with settings

		{
			composition c(t, i, w, e);
			c.start();
		}

		// Context goes out of scope, all good
	}
	
	
}
