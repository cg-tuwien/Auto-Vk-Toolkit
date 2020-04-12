#include <cg_base.hpp>

namespace cgb
{
	namespace settings
	{
		// Define the global variables:

		std::string gPhysicalDeviceSelectionHint = "";

		std::string gApplicationName = "cg_base Application";

		uint32_t gApplicationVersion = cgb::make_version(1u, 0u, 0u);

		std::vector<const char*> gRequiredInstanceExtensions;

		std::vector<const char*> gValidationLayersToBeActivated = {
			"VK_LAYER_KHRONOS_validation"
		};

		bool gEnableValidationLayersAlsoInReleaseBuilds = false;

		std::vector<const char*> gRequiredDeviceExtensions;

		device_queue_selection_strategy gQueueSelectionPreference = device_queue_selection_strategy::prefer_separate_queues;

		bool gPreferSameQueueForGraphicsAndPresent = true;

		uint32_t gDescriptorPoolSizeFactor = 3u;

		bool gLoadImagesInSrgbFormatByDefault = false;
	}
}
