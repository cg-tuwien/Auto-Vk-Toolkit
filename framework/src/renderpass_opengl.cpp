namespace cgb
{
	owning_resource<renderpass_t> renderpass_t::create(std::vector<attachment> pAttachments, cgb::context_specific_function<void(renderpass_t&)> pAlterConfigBeforeCreation)
	{
		renderpass_t result;
		return result; 
	}
}
