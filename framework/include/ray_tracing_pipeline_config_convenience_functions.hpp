#pragma once

namespace cgb
{
#pragma region shader_table_config convenience functions
	// End of recursive variadic template handling
	inline void add_shader_table_entry(shader_table_config& _ShaderTableConfig) { /* We're done here. */ }

	// Add a single shader (without hit group) to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& _ShaderTableConfig, shader_info _ShaderInfo, Ts... args)
	{
		_ShaderTableConfig.mShaderTableEntries.push_back(std::move(_ShaderInfo));
		add_shader_table_entry(_ShaderTableConfig, std::move(args)...);
	}

	// Add a single shader (without hit group) to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& _ShaderTableConfig, std::string_view _ShaderPath, Ts... args)
	{
		_ShaderTableConfig.mShaderTableEntries.push_back(shader_info::create(std::string(_ShaderPath)));
		add_shader_table_entry(_ShaderTableConfig, std::move(args)...);
	}

	// Add a triangles-intersection-type hit group to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& _ShaderTableConfig, triangles_hit_group _HitGroup, Ts... args)
	{
		_ShaderTableConfig.mShaderTableEntries.push_back(std::move(_HitGroup));
		add_shader_table_entry(_ShaderTableConfig, std::move(args)...);
	}

	// Add a procedural-type hit group to the shader table definition
	template <typename... Ts>
	void add_shader_table_entry(shader_table_config& _ShaderTableConfig, procedural_hit_group _HitGroup, Ts... args)
	{
		_ShaderTableConfig.mShaderTableEntries.push_back(std::move(_HitGroup));
		add_shader_table_entry(_ShaderTableConfig, std::move(args)...);
	}

	// Define a shader table which is to be used with a ray tracing pipeline
	template <typename... Ts>
	shader_table_config define_shader_table(Ts... args)
	{
		shader_table_config shaderTableConfig;
		add_shader_table_entry(shaderTableConfig, std::move(args)...);
		return shaderTableConfig;
	}
#pragma endregion

#pragma region ray_tracing_pipeline_config convenience functions
	// End of recursive variadic template handling
	inline void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, cfg::pipeline_settings _Setting, Ts... args)
	{
		_Config.mPipelineSettings |= _Setting;
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add the shader table to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, shader_table_config _ShaderTable, Ts... args)
	{
		_Config.mShaderTableConfig = std::move(_ShaderTable);
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add the maximum recursion setting to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, max_recursion_depth _MaxRecursionDepth, Ts... args)
	{
		_Config.mMaxRecursionDepth = std::move(_MaxRecursionDepth);
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, binding_data _ResourceBinding, Ts... args)
	{
		_Config.mResourceBindings.push_back(std::move(_ResourceBinding));
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add a push constants binding to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, push_constant_binding_data _PushConstBinding, Ts... args)
	{
		_Config.mPushConstantsBindings.push_back(std::move(_PushConstBinding));
		add_config(_Config, _Func, std::move(args)...);
	}

	// Add an config-alteration function to the pipeline config
	template <typename... Ts>
	void add_config(ray_tracing_pipeline_config& _Config, context_specific_function<void(ray_tracing_pipeline_t&)>& _Func, cgb::context_specific_function<void(ray_tracing_pipeline_t&)> _AlterConfigBeforeCreation, Ts... args)
	{
		_Func = std::move(_AlterConfigBeforeCreation);
		add_config(_Config, _Func, std::move(args)...);
	}

	/**	Convenience function for gathering the ray tracing pipeline's configuration.
	 *
	 *	It supports the following types:
	 *		- cgb::cfg::pipeline_settings
	 *		- cgb::shader_table_config (hint: use `cgb::define_shader_table`)
	 *		- cgb::max_recursion_depth
	 *		- cgb::binding_data
	 *		- cgb::push_constant_binding_data
	 *		- cgb::context_specific_function<void(ray_tracing_pipeline_t&)>
	 *
	 *	For building the shader table in a convenient fashion, use the `cgb::define_shader_table` function!
	 *	
	 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref ray_tracing_pipeline_t::create
	 */
	template <typename... Ts>
	owning_resource<ray_tracing_pipeline_t> ray_tracing_pipeline_for(Ts... args)
	{
		// 1. GATHER CONFIG
		context_specific_function<void(ray_tracing_pipeline_t&)> alterConfigFunction;
		ray_tracing_pipeline_config config;
		add_config(config, alterConfigFunction, std::move(args)...);

		// 2. CREATE PIPELINE according to the config
		// ============================================ Vk ============================================ 
		//    => VULKAN CODE HERE:
		return ray_tracing_pipeline_t::create(std::move(config), std::move(alterConfigFunction));
		// ============================================================================================ 
	}
#pragma endregion 
}
