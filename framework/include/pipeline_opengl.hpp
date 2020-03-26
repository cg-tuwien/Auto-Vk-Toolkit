#pragma once

namespace cgb
{
	/** TBD
	*/
	class graphics_pipeline_t
	{
	public:
		graphics_pipeline_t() = default;
		graphics_pipeline_t(graphics_pipeline_t&&) noexcept = default;
		graphics_pipeline_t(const graphics_pipeline_t&) = delete;
		graphics_pipeline_t& operator=(graphics_pipeline_t&&) noexcept = default;
		graphics_pipeline_t& operator=(const graphics_pipeline_t&) = delete;
		~graphics_pipeline_t() = default;

		static owning_resource<graphics_pipeline_t> create(graphics_pipeline_config _Config, cgb::context_specific_function<void(graphics_pipeline_t&)> _AlterConfigBeforeCreation = {});

	private:
		context_tracker<graphics_pipeline_t> mTracker;
	};

	using graphics_pipeline = owning_resource<graphics_pipeline_t>;
}
