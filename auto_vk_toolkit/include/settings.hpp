#pragma once
#include "auto_vk_toolkit.hpp"

namespace avk
{
	/** Set a hint about which device to select. This could be, e.g., "Intel" or "RTX". */
	struct physical_device_selection_hint
	{
		physical_device_selection_hint() {}
		physical_device_selection_hint(std::string aValue) : mValue{ std::move(aValue) } {}
		/** Set a filter text for physical device selection. */
		physical_device_selection_hint& set_filter_text(std::string aValue) { mValue = aValue; return *this; }
		std::string mValue = "";
	};

	/** Set this to your application's name */
	struct application_name
	{
		application_name(std::string aValue = "Vulkan application powered by Auto-Vk-Toolkit") : mValue{ std::move(aValue) } {}
		std::string mValue;
	};

	/** Set this to your application's version */
	struct application_version
	{
		application_version(uint32_t aValue = avk::make_version(1u, 0u, 0u)) : mValue{ aValue } {}
		uint32_t mValue;
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
		validation_layers() : mEnableInRelease{ false }
		{
			mLayers.push_back("VK_LAYER_KHRONOS_validation");
		}

		validation_layers& add_layer(const char* aLayerName)
		{
			auto it = std::find(std::begin(mLayers), std::end(mLayers), std::string(aLayerName));
			if (std::end(mLayers) == it) {
				mLayers.push_back(aLayerName);
			}
			else {
				LOG_INFO(std::format("Validation layer '{}' was already added.", std::string(aLayerName)));
			}
			return *this;
		}

		validation_layers& remove_layer(const char* aLayerName)
		{
			auto it = std::find(std::begin(mLayers), std::end(mLayers), std::string(aLayerName));
			if (std::end(mLayers) != it) {
				mLayers.erase(it);
			}
			else {
				LOG_INFO(std::format("Validation layer '{}' not found.", aLayerName));
			}
			return *this;
		}

		validation_layers& remove_all_layers()
		{
			mLayers.clear();
			return *this;
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

		validation_layers& enable_feature(vk::ValidationFeatureEnableEXT aFeature)
		{
			auto it = std::find(std::begin(mFeaturesToEnable), std::end(mFeaturesToEnable), aFeature);
			if (std::end(mFeaturesToEnable) == it) {
				mFeaturesToEnable.push_back(aFeature);
			}
			return *this;
		}

		validation_layers& disable_feature(vk::ValidationFeatureEnableEXT aFeature)
		{
			mFeaturesToEnable.erase(std::remove(std::begin(mFeaturesToEnable), std::end(mFeaturesToEnable), aFeature), std::end(mFeaturesToEnable));
			return *this;
		}

		validation_layers& enable_feature(vk::ValidationFeatureDisableEXT aFeature)
		{
			mFeaturesToDisable.erase(std::remove(std::begin(mFeaturesToDisable), std::end(mFeaturesToDisable), aFeature), std::end(mFeaturesToDisable));
			return *this;
		}

		validation_layers& disable_feature(vk::ValidationFeatureDisableEXT aFeature)
		{
			auto it = std::find(std::begin(mFeaturesToDisable), std::end(mFeaturesToDisable), aFeature);
			if (std::end(mFeaturesToDisable) == it) {
				mFeaturesToDisable.push_back(aFeature);
			}
			return *this;
		}

		std::vector<const char*> mLayers;
		std::vector<vk::ValidationFeatureEnableEXT> mFeaturesToEnable;
		std::vector<vk::ValidationFeatureDisableEXT> mFeaturesToDisable;
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

	/** Fill this vector with optional device extensions */
	struct optional_device_extensions
	{
		optional_device_extensions(const char* aExtensionName = nullptr)
		{
			if (nullptr != aExtensionName) {
				mExtensions.push_back(aExtensionName);
			}
		}

		optional_device_extensions& add_extension(const char* aExtensionName)
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

	/** Pass a function to modify the requested Vulkan 1.1 device features.
	 *	@typedef	F		void(vk::PhysicalDeviceVulkan11Features&)
	 *						Modify the values of the passed reference to vk::PhysicalDeviceVulkan11Features& directly!
	 */
	struct alter_vulkan11_device_features
	{
		template <typename F>
		alter_vulkan11_device_features(F&& aCallback) : mFunction{ std::forward(aCallback) }
		{ }
		
		std::function<void(vk::PhysicalDeviceVulkan11Features&)> mFunction;
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

	/**	A struct which contains a pNext pointer to be added to the VkPhysicalFeatures2 pNext chain.
	 *	Just let the only pNext member point to the address of a Vulkan configuration struct,
	 *	and leave its pNext pointer at nullptr.
	 *	The framework will internally take care of wiring together all the pNext pointers!
	 */
	struct physical_device_features_pNext_chain_entry
	{
		void* pNext;
	};

	struct settings
	{
		physical_device_selection_hint mPhysicalDeviceSelectionHint;
		application_name mApplicationName;
		application_version mApplicationVersion;
		required_instance_extensions mRequiredInstanceExtensions;
		validation_layers mValidationLayers;
		required_device_extensions mRequiredDeviceExtensions;
		optional_device_extensions mOptionalDeviceExtensions;
		vk::DebugUtilsMessageSeverityFlagsEXT mEnabledDebugUtilsMessageSeverities = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
		vk::DebugUtilsMessageTypeFlagsEXT mEnabledDebugUtilsMessageTypes = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
		std::vector<physical_device_features_pNext_chain_entry> mPhysicalDeviceFeaturesPNextChainEntries;
	};
}
