#pragma once
#include <exekutor.hpp>

namespace xk
{
	namespace settings
	{
		extern std::string gApplicationName;

		extern uint32_t gApplicationVersion;

		extern std::string gPhysicalDeviceSelectionHint;

		extern std::vector<const char*> gRequiredInstanceExtensions;

		extern std::vector<const char*> gValidationLayersToBeActivated;

		extern bool gEnableValidationLayersAlsoInReleaseBuilds;

		extern std::vector<const char*> gRequiredDeviceExtensions;

		extern bool gPreferSameQueueForGraphicsAndPresent;

		/**	A factor which is applied to the allocation sizes of descriptor pools.
		 *	Whenever a pool is created, this size factor is applied to its size.
		 *	Low factors can lead to the creation of more pools, high factors can
		 *	lead to overallocation. 
		 *	If you only create descriptor sets at initialization time, a sensible
		 *	factor would be the number of concurrent frames ("frames in flight").
		 */
		extern uint32_t gDescriptorPoolSizeFactor;

		extern bool gLoadImagesInSrgbFormatByDefault;

		extern bool gEnableBufferDeviceAddress;
	}

	inline vulkan& context()
	{
		static vulkan instance;
		return instance;
	}

}
