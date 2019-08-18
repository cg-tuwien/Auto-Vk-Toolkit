#pragma once

namespace cgb
{
	//extern descriptor_set_layout layout_for(std::initializer_list<binding_data> pBindings);




	// End of recursive variadic template handling
	inline void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func) { /* We're done here. */ }

	// Add a specific pipeline setting to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::pipeline_settings _Setting, Ts... args)
	{
		_Config.mPipelineSettings |= _Setting;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a complete render pass to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, renderpass _RenderPass, uint32_t _Subpass, Ts... args)
	{
		_Config.mRenderPassSubpass = std::move(std::make_tuple(std::move(_RenderPass), _Subpass));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a complete render pass to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, renderpass _RenderPass, Ts... args)
	{
		_Config.mRenderPassSubpass = std::move(std::make_tuple(std::move(_RenderPass), 0u)); // Default to the first subpass if none is specified
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a renderpass attachment to the (temporary) attachments vector and build renderpass afterwards
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, attachment _Attachment, Ts... args)
	{
		_Attachments.push_back(std::move(_Attachment));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add an input binding location to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, input_binding_location_data _InputBinding, Ts... args)
	{
		_Config.mInputBindingLocations.push_back(std::move(_InputBinding));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set the topology of the input attributes
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::primitive_topology _Topology, Ts... args)
	{
		_Config.mPrimitiveTopology = _Topology;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a shader to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, shader_info _ShaderInfo, Ts... args)
	{
		_Config.mShaderInfos.push_back(std::move(_ShaderInfo));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Accept a string and assume it refers to a shader file
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, std::string_view _ShaderPath, Ts... args)
	{
		_Config.mShaderInfos.push_back(shader_info::create(std::string(_ShaderPath)));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set the depth test behavior in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::depth_test _DepthTestConfig, Ts... args)
	{
		_Config.mDepthTestConfig = std::move(_DepthTestConfig);
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set the depth write behavior in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::depth_write _DepthWriteConfig, Ts... args)
	{
		_Config.mDepthWriteConfig = std::move(_DepthWriteConfig);
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a viewport, depth, and scissors entry to the pipeline configuration
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::viewport_depth_scissors_config _ViewportDepthScissorsConfig, Ts... args)
	{
		_Config.mViewportDepthConfig.push_back(std::move(_ViewportDepthScissorsConfig));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set the culling mode in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::culling_mode _CullingMode, Ts... args)
	{
		_Config.mCullingMode = _CullingMode;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set the definition of front faces in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::front_face _FrontFace, Ts... args)
	{
		_Config.mFrontFaceWindingOrder = std::move(_FrontFace);
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set how to draw polygons in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::polygon_drawing _PolygonDrawingConfig, Ts... args)
	{
		_Config.mPolygonDrawingModeAndConfig = std::move(_PolygonDrawingConfig);
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Set how the rasterizer handles geometry in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::rasterizer_geometry_mode _RasterizerGeometryMode, Ts... args)
	{
		_Config.mRasterizerGeometryMode = _RasterizerGeometryMode;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Sets if there should be some special depth handling in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::depth_clamp_bias _DepthSettings, Ts... args)
	{
		_Config.mDepthClampBiasConfig = _DepthSettings;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Sets some color blending parameters in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::color_blending_settings _ColorBlendingSettings, Ts... args)
	{
		_Config.mColorBlendingSettings = _ColorBlendingSettings;
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Sets some color blending parameters in the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cfg::color_blending_config _ColorBlendingConfig, Ts... args)
	{
		_Config.mColorBlendingPerAttachment.push_back(std::move(_ColorBlendingConfig));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, binding_data _ResourceBinding, Ts... args)
	{
		_Config.mResourceBindings.push_back(std::move(_ResourceBinding));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, push_constant_binding_data _PushConstBinding, Ts... args)
	{
		_Config.mPushConstantsBindings.push_back(std::move(_PushConstBinding));
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	// Add a resource binding to the pipeline config
	template <typename... Ts>
	void add_config(graphics_pipeline_config& _Config, std::vector<attachment>& _Attachments, context_specific_function<void(graphics_pipeline_t&)>& _Func, cgb::context_specific_function<void(graphics_pipeline_t&)> _AlterConfigBeforeCreation, Ts... args)
	{
		_Func = std::move(_AlterConfigBeforeCreation);
		add_config(_Config, _Attachments, _Func, std::move(args)...);
	}

	/**	Convenience function for gathering the graphic pipeline's configuration.
	 *	
	 *	It supports the following types 
	 *
	 *
	 *	For the actual Vulkan-calls which finally create the pipeline, please refer to @ref graphics_pipeline_t::create
	 */
	template <typename... Ts>
	owning_resource<graphics_pipeline_t> graphics_pipeline_for(Ts... args)
	{
		// 1. GATHER CONFIG
		std::vector<attachment> renderPassAttachments;
		context_specific_function<void(graphics_pipeline_t&)> alterConfigFunction;
		graphics_pipeline_config config;
		add_config(config, renderPassAttachments, alterConfigFunction, std::move(args)...);

		// Check if render pass attachments are in renderPassAttachments XOR config => only in that case, it is clear how to proceed, fail in other cases
		if (renderPassAttachments.size() > 0 == (config.mRenderPassSubpass.has_value() && nullptr != std::get<renderpass>(*config.mRenderPassSubpass)->handle())) {
			if (renderPassAttachments.size() == 0) {
				throw std::runtime_error("No renderpass config provided! Please provide a renderpass or attachments!");
			}
			throw std::runtime_error("Ambiguous renderpass config! Either set a renderpass XOR provide attachments!");
		}
		// ^ that was the sanity check. See if we have to build the renderpass from the attachments:
		if (renderPassAttachments.size() > 0) {
			add_config(config, renderPassAttachments, alterConfigFunction, renderpass_t::create(std::move(renderPassAttachments)));
		}

		// 2. CREATE PIPELINE according to the config
		// ============================================ Vk ============================================ 
		//    => VULKAN CODE HERE:
		return graphics_pipeline_t::create(std::move(config), std::move(alterConfigFunction));
		// ============================================================================================ 
	}
}