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

	/** Pass a function to modify the requested physical device features.
	 *	@typedef	F		void(vk::PhysicalDeviceFeatures&)
	 *						Modify the values of the passed reference to vk::PhysicalDeviceFeatures& directly!
	 */
	struct alter_requested_physical_device_features
	{
		template <typename F>
		alter_requested_physical_device_features(F&& aCallback) : mFunction{ std::forward(aCallback) }
		{ }
		
		std::function<void(vk::PhysicalDeviceFeatures&)> mFunction;
	};

	/** Pass a function to modify the requested Vulkan 1.2 device features.
	 *	@typedef	F		void(vk::PhysicalDeviceVulkan12Features&)
	 *						Modify the values of the passed reference to vk::PhysicalDeviceVulkan12Features& directly!
	 */
	struct alter_vulkan12_device_features
	{
		template <typename F>
		alter_vulkan12_device_features(F&& aCallback) : mFunction{ std::forward(aCallback) }
		{ }
		
		std::function<void(vk::PhysicalDeviceVulkan12Features&)> mFunction;
	};

	struct settings
	{
		physical_device_selection_hint mPhysicalDeviceSelectionHint;
		application_name mApplicationName;
		application_version mApplicationVersion;
		required_instance_extensions mRequiredInstanceExtensions;
		validation_layers mValidationLayers;
		required_device_extensions mRequiredDeviceExtensions;
	};
}
