#include <exekutor.hpp>

namespace xk
{
	namespace settings
	{
		// Define the global variables:

		std::string gPhysicalDeviceSelectionHint = "";

		std::string gApplicationName = "cg_base Application";

		uint32_t gApplicationVersion = ak::make_version(1u, 0u, 0u);

		std::vector<const char*> gRequiredInstanceExtensions;

		std::vector<const char*> gValidationLayersToBeActivated = {
			"VK_LAYER_KHRONOS_validation"
		};

		bool gEnableValidationLayersAlsoInReleaseBuilds = false;

		std::vector<const char*> gRequiredDeviceExtensions;

		bool gPreferSameQueueForGraphicsAndPresent = true;

		uint32_t gDescriptorPoolSizeFactor = 3u;

		bool gLoadImagesInSrgbFormatByDefault = false;

		bool gEnableBufferDeviceAddress = false;
	}
}
