#pragma once

namespace cgb
{
	namespace settings
	{
		/** Set this to your application's name */
		extern std::string gApplicationName;

		/** Set this to your application's version */
		extern uint32_t gApplicationVersion;

		/** Fill this vector with further required instance extensions, if required */
		extern std::vector<const char*> gRequiredInstanceExtensions;

		/** Modify this vector or strings according to your needs. 
		 *  By default, it will be initialized with a default validation layer name.
		 */
		extern std::vector<const char*> gValidationLayersToBeActivated;

		/** Fill this vector with required device extensions, if required */
		extern std::vector<const char*> gRequiredDeviceExtensions;

		/** Set this flag to true to opt out of using IMGUI, 
		 *	i.e. if true => not initialize it, not display it, nothing. 
		 *	The default for this flag is false, i.e. IMGUI is enabled.
		 */
		extern bool gDisableImGui;

		/** Configure how the queues should be selected.
		 *	Shall there be one queue for everything or rather
		 *	separate queues for everything? 
		 *  The default is `prefer_separate_queues`
		 */
		extern device_queue_selection_strategy gQueueSelectionPreference;

		/** Regardless of the `gQueueSelectionPreference` setting,
		 *	state that the same queue is preferred for graphics and present.
		 *	The default value is `true`, i.e. ignore the `gQueueSelectionPreference` setting.
		 */
		extern bool gPreferSameQueueForGraphicsAndPresent;

		/**	A factor which is applied to the allocation sizes of descriptor pools.
		 *	Whenever a pool is created, this size factor is applied to its size.
		 *	Low factors can lead to the creation of more pools, high factors can
		 *	lead to overallocation. 
		 *	If you only create descriptor sets at initialization time, a sensible
		 *	factor would be the number of concurrent frames ("frames in flight").
		 */
		extern uint32_t gDescriptorPoolSizeFactor;
	}

#if defined(USE_OPENGL46_CONTEXT)
	inline opengl46& context()
	{
		static opengl46 instance;
		return instance;
	}
#elif defined(USE_VULKAN_CONTEXT)
	inline vulkan& context()
	{
		static vulkan instance;
		return instance;
	}
#endif

}
