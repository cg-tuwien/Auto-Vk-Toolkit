#include <cg_base.hpp>

namespace cgb
{
	owning_resource<compute_pipeline_t> compute_pipeline_t::create(compute_pipeline_config _Config, cgb::context_specific_function<void(compute_pipeline_t&)> _AlterConfigBeforeCreation)
	{
		compute_pipeline_t result;

		// 1. Compile and store the one and only shader:
		if (!_Config.mShaderInfo.has_value()) {
			throw std::logic_error("Shader missing in compute_pipeline_config! A compute pipeline can not be constructed without a shader.");
		}
		//    Compile the shader
		result.mShader = std::move(shader::create(_Config.mShaderInfo.value()));
		assert(result.mShader.has_been_built());
		//    Just fill in the create struct
		result.mShaderStageCreateInfo = vk::PipelineShaderStageCreateInfo{}
			.setStage(to_vk_shader_stage(result.mShader.info().mShaderType))
			.setModule(result.mShader.handle())
			.setPName(result.mShader.info().mEntryPoint.c_str());
		
		// 2. Flags
		// TODO: Support all flags (only one of the flags is handled at the moment)
		result.mPipelineCreateFlags = {};
		if ((_Config.mPipelineSettings & cfg::pipeline_settings::disable_optimization) == cfg::pipeline_settings::disable_optimization) {
			result.mPipelineCreateFlags |= vk::PipelineCreateFlagBits::eDisableOptimization;
		}

		// 3. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(_Config.mResourceBindings));
		result.mAllDescriptorSetLayouts.allocate_all();
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(_Config.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : _Config.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stages(pcBinding.mShaderStages))
				.setOffset(pcBinding.mOffset)
				.setSize(pcBinding.mSize)
			);
			// TODO: Push Constants need a prettier interface
		}
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size()))
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 4. Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		// Create the PIPELINE LAYOUT
		result.mPipelineLayout = context().logical_device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::ComputePipelineCreateInfo{}
			.setFlags(result.mPipelineCreateFlags)
			.setStage(result.mShaderStageCreateInfo)
			.setLayout(result.layout_handle())
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional
		result.mPipeline = context().logical_device().createComputePipelineUnique(nullptr, pipelineInfo);

		result.mTracker.setTrackee(result);
		return result;
	}
}
