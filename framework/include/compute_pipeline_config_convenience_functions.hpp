#pragma once

namespace cgb
{
	// End of recursive variadic template handling
	inline void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, cfg::pipeline_settings _Setting, Ts... args)
	{
		_Config.mPipelineSettings |= _Setting;
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add a shader to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, shader_info _ShaderInfo, Ts... args)
	{
		if (_Config.mShaderInfo.has_value()) {
			throw cgb::logic_error("Only one shader is supported for a compute pipeline.");
		}
		if (_ShaderInfo.mShaderType != shader_type::compute) {
			throw cgb::logic_error("The shader's type is not compute.");
		}
		_Config.mShaderInfo = std::move(_ShaderInfo);
		add_config(_Config, _Func, std::move(args)...);
	}

	// Accept a string and assume it refers to a shader file
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, std::string_view _ShaderPath, Ts... args)
	{
		if (_Config.mShaderInfo.has_value()) {
			throw cgb::logic_error("Only one shader is supported for a compute pipeline.");
		}
		_Config.mShaderInfo = shader_info::create(std::string(_ShaderPath));
		if (_Config.mShaderInfo->mShaderType != shader_type::compute) {
			throw cgb::logic_error("The shader's type is not compute.");
		}
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, binding_data _ResourceBinding, Ts... args)
	{
		if ((_ResourceBinding.mLayoutBinding.stageFlags & vk::ShaderStageFlagBits::eCompute) != vk::ShaderStageFlagBits::eCompute) {
			throw cgb::logic_error("Resource not visible in compute shader, but this is a compute pipeline => that makes no sense.");
		}
		_Config.mResourceBindings.push_back(std::move(_ResourceBinding));
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add a push constants binding to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, push_constant_binding_data _PushConstBinding, Ts... args)
	{
		if ((_PushConstBinding.mShaderStages & shader_type::compute) != shader_type::compute) {
			throw cgb::logic_error("Push constants are not visible in compute shader, but this is a compute pipeline => that makes no sense.");
		}
		_Config.mPushConstantsBindings.push_back(std::move(_PushConstBinding));
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add an config-alteration function to the pipeline config
	template <typename... Ts>
	void add_config(compute_pipeline_config& _Config, context_specific_function<void(compute_pipeline_t&)>& _Func, cgb::context_specific_function<void(compute_pipeline_t&)> _AlterConfigBeforeCreation, Ts... args)
	{
		_Func = std::move(_AlterConfigBeforeCreation);
		add_config(_Config, _Func, std::move(args)...);
	}

	/**	Convenience function for gathering the compute pipeline's configuration.
	 *
	 *	It supports the following types
	 *		- cgb::cfg::pipeline_settings
	 *		- cgb::shader_info or std::string_view
	 *		- cgb::binding_data
	 *		- cgb::push_constant_binding_data
	 *		- cgb::context_specific_function<void(compute_pipeline_t&)>
	 *
	 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref compute_pipeline_t::create
	 */
	template <typename... Ts>
	owning_resource<compute_pipeline_t> compute_pipeline_for(Ts... args)
	{
		// 1. GATHER CONFIG
		context_specific_function<void(compute_pipeline_t&)> alterConfigFunction;
		compute_pipeline_config config;
		add_config(config, alterConfigFunction, std::move(args)...);

		// 2. CREATE PIPELINE according to the config
		// ============================================ Vk ============================================ 
		//    => VULKAN CODE HERE:
		return compute_pipeline_t::create(std::move(config), std::move(alterConfigFunction));
		// ============================================================================================ 
	}
}
