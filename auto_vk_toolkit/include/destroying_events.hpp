#pragma once

#include "event.hpp"

namespace avk
{
	/** This event occurs just before a graphics pipeline is being destroyed, because
	 *	it was updated through the updater and replaced with a new one (which happened
	 *	most likely one or several frames ago).
	 */
	class destroying_graphics_pipeline_event : public event
	{
	public:
		bool update(event_data& aData) override
		{
			return aData.mGraphicsPipelinesToBeCleanedUp.size() > 0;
		}
	};

	static bool operator==(const destroying_graphics_pipeline_event& left, const destroying_graphics_pipeline_event& right) { return true; }
	static bool operator!=(const destroying_graphics_pipeline_event& left, const destroying_graphics_pipeline_event& right) { return false; }

	
	/** This event occurs just before a compute pipeline is being destroyed, because
	 *	it was updated through the updater and replaced with a new one (which happened
	 *	most likely one or several frames ago).
	 */
	class destroying_compute_pipeline_event : public event
	{
	public:
		bool update(event_data& aData) override
		{
			return aData.mComputePipelinesToBeCleanedUp.size() > 0;
		}
	};

	static bool operator==(const destroying_compute_pipeline_event& left, const destroying_compute_pipeline_event& right) { return true; }
	static bool operator!=(const destroying_compute_pipeline_event& left, const destroying_compute_pipeline_event& right) { return false; }

	
	/** This event occurs just before a ray tracing pipeline is being destroyed, because
	 *	it was updated through the updater and replaced with a new one (which happened
	 *	most likely one or several frames ago).
	 */
	class destroying_ray_tracing_pipeline_event : public event
	{
	public:
		bool update(event_data& aData) override
		{
			return aData.mRayTracingPipelinesToBeCleanedUp.size() > 0;
		}
	};

	static bool operator==(const destroying_ray_tracing_pipeline_event& left, const destroying_ray_tracing_pipeline_event& right) { return true; }
	static bool operator!=(const destroying_ray_tracing_pipeline_event& left, const destroying_ray_tracing_pipeline_event& right) { return false; }

	
	/** This event occurs just before an image is being destroyed, because
	 *	it was updated through the updater and replaced with a new one (which happened
	 *	most likely one or several frames ago).
	 */
	class destroying_image_event : public event
	{
	public:
		bool update(event_data& aData) override
		{
			return aData.mImagesToBeCleanedUp.size() > 0;
		}
	};

	static bool operator==(const destroying_image_event& left, const destroying_image_event& right) { return true; }
	static bool operator!=(const destroying_image_event& left, const destroying_image_event& right) { return false; }

	
	/** This event occurs just before an image view is being destroyed, because
	 *	it was updated through the updater and replaced with a new one (which happened
	 *	most likely one or several frames ago).
	 */
	class destroying_image_view_event : public event
	{
	public:
		bool update(event_data& aData) override
		{
			return aData.mImageViewsToBeCleanedUp.size() > 0;
		}

	};

	static bool operator==(const destroying_image_view_event& left, const destroying_image_view_event& right) { return true; }
	static bool operator!=(const destroying_image_view_event& left, const destroying_image_view_event& right) { return false; }
}

namespace std // Inject hash for destroying*events into std::
{
	template<> struct hash<avk::destroying_graphics_pipeline_event>
	{
		std::size_t operator()(avk::destroying_graphics_pipeline_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, std::string("destroying_graphics_pipeline_event"));
			return h;
		}
	};

	template<> struct hash<avk::destroying_compute_pipeline_event>
	{
		std::size_t operator()(avk::destroying_compute_pipeline_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, std::string("destroying_compute_pipeline_event"));
			return h;
		}
	};

	template<> struct hash<avk::destroying_ray_tracing_pipeline_event>
	{
		std::size_t operator()(avk::destroying_ray_tracing_pipeline_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, std::string("destroying_graphics_pipeline_event"));
			return h;
		}
	};

	template<> struct hash<avk::destroying_image_event>
	{
		std::size_t operator()(avk::destroying_image_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, std::string("destroying_image_event"));
			return h;
		}
	};

	template<> struct hash<avk::destroying_image_view_event>
	{
		std::size_t operator()(avk::destroying_image_view_event const& o) const noexcept
		{
			std::size_t h = 0;
			avk::hash_combine(h, std::string("destroying_image_view_event"));
			return h;
		}
	};
}
