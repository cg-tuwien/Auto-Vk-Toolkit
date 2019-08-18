#pragma once

namespace cgb
{
	/** TBD
	 */
	class renderpass_t
	{
	public:
		renderpass_t() = default;
		renderpass_t(renderpass_t&&) = default;
		renderpass_t(const renderpass_t&) = delete;
		renderpass_t& operator=(renderpass_t&&) = default;
		renderpass_t& operator=(const renderpass_t&) = delete;
		~renderpass_t() = default;

		static owning_resource<renderpass_t> create(std::vector<attachment> pAttachments, cgb::context_specific_function<void(renderpass_t&)> pAlterConfigBeforeCreation = {});

		context_tracker<renderpass_t> mTracker;
	};

	using renderpass = owning_resource<renderpass_t>;

}