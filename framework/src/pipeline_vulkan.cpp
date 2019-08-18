namespace cgb
{
	using namespace cpplinq;
	using namespace cgb::cfg;

	owning_resource<graphics_pipeline_t> graphics_pipeline_t::create(graphics_pipeline_config _Config, cgb::context_specific_function<void(graphics_pipeline_t&)> _AlterConfigBeforeCreation)
	{
		graphics_pipeline_t result;

		// 0. Own the renderpass
		{
			assert(_Config.mRenderPassSubpass.has_value());
			auto [rp, sp] = std::move(_Config.mRenderPassSubpass.value());
			result.mRenderPass = std::move(rp);
			result.mSubpassIndex = sp;
		}

		// 1. Compile the array of vertex input binding descriptions
		{ 
			// Select DISTINCT bindings:
			auto bindings = from(_Config.mInputBindingLocations)
				>> select([](const input_binding_location_data& _BindingData) { return _BindingData.mGeneralData; })
				>> distinct() // see what I did there
				>> orderby([](const input_binding_general_data& _GeneralData) { return _GeneralData.mBinding; })
				>> to_vector();
			result.mVertexInputBindingDescriptions.reserve(bindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!

			for (auto& bindingData : bindings) {

				const auto numRecordsWithSameBinding = std::count_if(std::begin(bindings), std::end(bindings), 
					[bindingId = bindingData.mBinding](const input_binding_general_data& _GeneralData) {
						return _GeneralData.mBinding == bindingId;
					});
				if (1 != numRecordsWithSameBinding) {
					throw std::runtime_error(fmt::format("The input binding {} is defined in different ways. Make sure to define it uniformly across different bindings/attribute descriptions!", bindingData.mBinding));
				}

				result.mVertexInputBindingDescriptions.push_back(vk::VertexInputBindingDescription()
					// The following parameters are guaranteed to be the same. We have checked this.
					.setBinding(bindingData.mBinding)
					.setStride(bindingData.mStride)
					.setInputRate(to_vk_vertex_input_rate(bindingData.mKind))
					// Don't need the location here
				);
			}
		}

		// 2. Compile the array of vertex input attribute descriptions
		//  They will reference the bindings created in step 1.
		result.mVertexInputAttributeDescriptions.reserve(_Config.mInputBindingLocations.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& attribData : _Config.mInputBindingLocations) {
			result.mVertexInputAttributeDescriptions.push_back(vk::VertexInputAttributeDescription()
				.setBinding(attribData.mGeneralData.mBinding)
				.setLocation(attribData.mMemberMetaData.mLocation)
				.setFormat(attribData.mMemberMetaData.mFormat.mFormat)
				.setOffset(static_cast<uint32_t>(attribData.mMemberMetaData.mOffset))
			);
		}

		// 3. With the data from 1. and 2., create the complete vertex input info struct, passed to the pipeline creation
		result.mPipelineVertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(static_cast<uint32_t>(result.mVertexInputBindingDescriptions.size()))
			.setPVertexBindingDescriptions(result.mVertexInputBindingDescriptions.data())
			.setVertexAttributeDescriptionCount(static_cast<uint32_t>(result.mVertexInputAttributeDescriptions.size()))
			.setPVertexAttributeDescriptions(result.mVertexInputAttributeDescriptions.data());

		// 4. Set how the data (from steps 1.-3.) is to be interpreted (e.g. triangles, points, lists, patches, etc.)
		result.mInputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(to_vk_primitive_topology(_Config.mPrimitiveTopology))
			.setPrimitiveRestartEnable(VK_FALSE);

		// 5. Compile and store the shaders:
		result.mShaders.reserve(_Config.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		result.mShaderStageCreateInfos.reserve(_Config.mShaderInfos.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (auto& shaderInfo : _Config.mShaderInfos) {
			// 5.1 Compile the shader
			result.mShaders.push_back(std::move(shader::create(shaderInfo)));
			assert(result.mShaders.back().has_been_built());
			// 5.2 Combine
			result.mShaderStageCreateInfos.push_back(vk::PipelineShaderStageCreateInfo{}
				.setStage(to_vk_shader_stage(result.mShaders.back().info().mShaderType))
				.setModule(result.mShaders.back().handle())
				.setPName(result.mShaders.back().info().mEntryPoint.c_str())
			);
		}

		// 6. Viewport configuration
		{
			// 6.1 Viewport and depth configuration(s):
			result.mViewports.reserve(_Config.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			result.mScissors.reserve(_Config.mViewportDepthConfig.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (auto& vp : _Config.mViewportDepthConfig) {
				result.mViewports.push_back(vk::Viewport{}
					.setX(vp.x())
					.setY(vp.y())
					.setWidth(vp.width())
					.setHeight(vp.height())
					.setMinDepth(vp.min_depth())
					.setMaxDepth(vp.max_depth())
				);
				// 6.2 Skip scissors for now
				// TODO: Implement scissors support properly
				result.mScissors.push_back(vk::Rect2D{}
					.setOffset({static_cast<int32_t>(vp.x()), static_cast<int32_t>(vp.y())})
					.setExtent({static_cast<uint32_t>(vp.width()), static_cast<uint32_t>(vp.height())})
				);
			}
			// 6.3 Add everything together
			result.mViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{}
				.setViewportCount(static_cast<uint32_t>(result.mViewports.size()))
				.setPViewports(result.mViewports.data())
				.setScissorCount(static_cast<uint32_t>(result.mScissors.size()))
				.setPScissors(result.mScissors.data());
		}

		// 7. Rasterization state
		result.mRasterizationStateCreateInfo =  vk::PipelineRasterizationStateCreateInfo{}
			// Various, but important settings:
			.setRasterizerDiscardEnable(to_vk_bool(_Config.mRasterizerGeometryMode == rasterizer_geometry_mode::discard_geometry))
			.setPolygonMode(to_vk_polygon_mode(_Config.mPolygonDrawingModeAndConfig.drawing_mode()))
			.setLineWidth(_Config.mPolygonDrawingModeAndConfig.line_width())
			.setCullMode(to_vk_cull_mode(_Config.mCullingMode))
			.setFrontFace(to_vk_front_face(_Config.mFrontFaceWindingOrder.winding_order_of_front_faces()))
			// Depth-related settings:
			.setDepthClampEnable(to_vk_bool(_Config.mDepthClampBiasConfig.is_clamp_to_frustum_enabled()))
			.setDepthBiasEnable(to_vk_bool(_Config.mDepthClampBiasConfig.is_depth_bias_enabled()))
			.setDepthBiasConstantFactor(_Config.mDepthClampBiasConfig.bias_constant_factor())
			.setDepthBiasClamp(_Config.mDepthClampBiasConfig.bias_clamp_value())
			.setDepthBiasSlopeFactor(_Config.mDepthClampBiasConfig.bias_slope_factor());

		// 8. Depth-stencil config
		result.mDepthStencilConfig = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(to_vk_bool(_Config.mDepthTestConfig.is_enabled()))
			.setDepthCompareOp(to_vk_compare_op(_Config.mDepthTestConfig.depth_compare_operation()))
			.setDepthWriteEnable(to_vk_bool(_Config.mDepthWriteConfig.is_enabled()))
			.setDepthBoundsTestEnable(to_vk_bool(_Config.mDepthBoundsConfig.is_enabled()))
			.setMinDepthBounds(_Config.mDepthBoundsConfig.min_bounds())
			.setMaxDepthBounds(_Config.mDepthBoundsConfig.max_bounds())
			.setStencilTestEnable(VK_FALSE); // TODO: Add support for stencil testing

		// 9. Color Blending
		{ 
			// Do we have an "universal" color blending config? That means, one that is not assigned to a specific color target attachment id.
			auto universalConfig = from(_Config.mColorBlendingPerAttachment)
				>> where([](const color_blending_config& config) { return !config.mTargetAttachment.has_value(); })
				>> to_vector();

			if (universalConfig.size() > 1) {
				throw std::runtime_error("Ambiguous 'universal' color blending configurations. Either provide only one 'universal' "
					"config (which is not attached to a specific color target) or assign them to specific color target attachment ids.");
			}

			// Iterate over all color target attachments and set a color blending config
			const auto n = (*result.mRenderPass).color_attachments().size();
			result.mBlendingConfigsForColorAttachments.reserve(n); // Important! Otherwise the vector might realloc and .data() will become invalid!
			for (size_t i = 0; i < n; ++i) {
				// Do we have a specific blending config for color attachment i?
				auto configForI = from(_Config.mColorBlendingPerAttachment)
					>> where([i](const color_blending_config& config) { return config.mTargetAttachment.has_value() && config.mTargetAttachment.value() == i; })
					>> to_vector();
				if (configForI.size() > 1) {
					throw std::runtime_error(fmt::format("Ambiguous color blending configuration for color attachment at index {}. Provide only one config per color attachment!", i));
				}
				// Determine which color blending to use for this attachment:
				color_blending_config toUse = configForI.size() == 1 ? configForI[0] : color_blending_config::disable();
				result.mBlendingConfigsForColorAttachments.push_back(vk::PipelineColorBlendAttachmentState()
					.setColorWriteMask(to_vk_color_components(toUse.affected_color_channels()))
					.setBlendEnable(to_vk_bool(toUse.is_blending_enabled())) // If blendEnable is set to VK_FALSE, then the new color from the fragment shader is passed through unmodified. [4]
					.setSrcColorBlendFactor(to_vk_blend_factor(toUse.color_source_factor())) 
					.setDstColorBlendFactor(to_vk_blend_factor(toUse.color_destination_factor()))
					.setColorBlendOp(to_vk_blend_operation(toUse.color_operation()))
					.setSrcAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_source_factor()))
					.setDstAlphaBlendFactor(to_vk_blend_factor(toUse.alpha_destination_factor()))
					.setAlphaBlendOp(to_vk_blend_operation(toUse.alpha_operation()))
				);
			}

			// General blending settings and reference to the array of color attachment blending configs
			result.mColorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
				.setLogicOpEnable(to_vk_bool(_Config.mColorBlendingSettings.is_logic_operation_enabled())) // If you want to use the second method of blending (bitwise combination), then you should set logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field. [4]
				.setLogicOp(to_vk_logic_operation(_Config.mColorBlendingSettings.logic_operation())) 
				.setAttachmentCount(static_cast<uint32_t>(result.mBlendingConfigsForColorAttachments.size()))
				.setPAttachments(result.mBlendingConfigsForColorAttachments.data())
				.setBlendConstants({
					{
						_Config.mColorBlendingSettings.blend_constants().r,
						_Config.mColorBlendingSettings.blend_constants().g,
						_Config.mColorBlendingSettings.blend_constants().b,
						_Config.mColorBlendingSettings.blend_constants().a,
					} 
				});
		}

		// 10. Multisample state
		// TODO: Can the settings be inferred from the renderpass' color attachments (as they are right now)? If they can't, how to handle this situation? 
		{
			vk::SampleCountFlagBits numSamples = vk::SampleCountFlagBits::e1;

			// See what is configured in the render pass
			auto colorAttConfigs = from ((*result.mRenderPass).color_attachments())
				>> where ([](const vk::AttachmentReference& colorAttachment) { return colorAttachment.attachment != VK_ATTACHMENT_UNUSED; })
				// The color_attachments() contain indices of the actual attachment_descriptions() => select the latter!
				>> select ([&rp = (*result.mRenderPass)](const vk::AttachmentReference& colorAttachment) { return rp.attachment_descriptions()[colorAttachment.attachment]; })
				>> to_vector();

			if (colorAttConfigs.size() > 0) {
				numSamples = colorAttConfigs[0].samples;
			}

#if defined(_DEBUG) 
			for (const vk::AttachmentDescription& config: colorAttConfigs) {
				if (config.samples != numSamples) {
					LOG_WARNING("Not all of the color target attachments have the same number of samples configured.");
				}
			}
#endif

			result.mMultisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
				.setSampleShadingEnable(vk::SampleCountFlagBits::e1 == numSamples ? VK_FALSE : VK_TRUE) // disable/enable?
				.setRasterizationSamples(numSamples)
				.setMinSampleShading(1.0f) // Optional
				.setPSampleMask(nullptr) // Optional
				.setAlphaToCoverageEnable(VK_FALSE) // Optional
				.setAlphaToOneEnable(VK_FALSE); // Optional
			// TODO: That is probably not enough for every case. Further customization options should be added!
		}

		// 11. Dynamic state
		{
			// Don't need to pre-alloc the storage for this one

			// Check for viewport dynamic state
			for (const auto& vpdc : _Config.mViewportDepthConfig) {
				if (vpdc.is_dynamic_viewport_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eViewport);
				}
			}
			// Check for scissor dynamic state
			for (const auto& vpdc : _Config.mViewportDepthConfig) {
				if (vpdc.is_dynamic_scissor_enabled())	{
					result.mDynamicStateEntries.push_back(vk::DynamicState::eScissor);
				}
			}
			// Check for dynamic line width
			if (_Config.mPolygonDrawingModeAndConfig.dynamic_line_width()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eLineWidth);
			}
			// Check for dynamic depth bias
			if (_Config.mDepthClampBiasConfig.is_dynamic_depth_bias_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBias);
			}
			// Check for dynamic depth bounds
			if (_Config.mDepthBoundsConfig.is_dynamic_depth_bounds_enabled()) {
				result.mDynamicStateEntries.push_back(vk::DynamicState::eDepthBounds);
			}
			// TODO: Support further dynamic states
		}

		// 12. Flags
		// TODO: Support flags
		result.mPipelineCreateFlags = {};

		// 13. Compile the PIPELINE LAYOUT data and create-info
		// Get the descriptor set layouts
		result.mAllDescriptorSetLayouts = set_of_descriptor_set_layouts::prepare(std::move(_Config.mResourceBindings));
		result.mAllDescriptorSetLayouts.allocate_all();
		auto descriptorSetLayoutHandles = result.mAllDescriptorSetLayouts.layout_handles();
		// Gather the push constant data
		result.mPushConstantRanges.reserve(_Config.mPushConstantsBindings.size()); // Important! Otherwise the vector might realloc and .data() will become invalid!
		for (const auto& pcBinding : _Config.mPushConstantsBindings) {
			result.mPushConstantRanges.push_back(vk::PushConstantRange{}
				.setStageFlags(to_vk_shader_stage(pcBinding.mShaderStages))
				.setOffset(pcBinding.mOffset)
				.setSize(pcBinding.mSize)
			);
			// TODO: Push Constants need a prettier interface
		}
		// These uniform values (Anm.: passed to shaders) need to be specified during pipeline creation by creating a VkPipelineLayout object. [4]
		result.mPipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo{}
			.setSetLayoutCount(static_cast<uint32_t>(descriptorSetLayoutHandles.size()))
			.setPSetLayouts(descriptorSetLayoutHandles.data())
			.setPushConstantRangeCount(static_cast<uint32_t>(result.mPushConstantRanges.size())) 
			.setPPushConstantRanges(result.mPushConstantRanges.data());

		// 14. Maybe alter the config?!
		if (_AlterConfigBeforeCreation.mFunction) {
			_AlterConfigBeforeCreation.mFunction(result);
		}

		// Create the PIPELINE LAYOUT
		result.mPipelineLayout = context().logical_device().createPipelineLayoutUnique(result.mPipelineLayoutCreateInfo);
		assert(nullptr != result.layout_handle());

		assert (_Config.mRenderPassSubpass.has_value());
		// Create the PIPELINE, a.k.a. putting it all together:
		auto pipelineInfo = vk::GraphicsPipelineCreateInfo()
			// 0. Render Pass
			.setRenderPass((*result.mRenderPass).handle())
			.setSubpass(result.mSubpassIndex)
			// 1., 2., and 3.
			.setPVertexInputState(&result.mPipelineVertexInputStateCreateInfo)
			// 4.
			.setPInputAssemblyState(&result.mInputAssemblyStateCreateInfo)
			// 5.
			.setStageCount(static_cast<uint32_t>(result.mShaderStageCreateInfos.size()))
			.setPStages(result.mShaderStageCreateInfos.data())
			// 6.
			.setPViewportState(&result.mViewportStateCreateInfo)
			// 7.
			.setPRasterizationState(&result.mRasterizationStateCreateInfo)
			// 8.
			.setPDepthStencilState(&result.mDepthStencilConfig)
			// 9.
			.setPColorBlendState(&result.mColorBlendStateCreateInfo)
			// 10.
			.setPMultisampleState(&result.mMultisampleStateCreateInfo)
			// 11.
			.setPDynamicState(result.mDynamicStateEntries.size() == 0 ? nullptr : &result.mDynamicStateCreateInfo) // Optional
			// 12.
			.setFlags(result.mPipelineCreateFlags)
			// TODO: Proceed here
			.setLayout(result.layout_handle())
			// Base pipeline:
			.setBasePipelineHandle(nullptr) // Optional
			.setBasePipelineIndex(-1); // Optional

		result.mPipeline = context().logical_device().createGraphicsPipelineUnique(nullptr, pipelineInfo);
		result.mTracker.setTrackee(result);
		return result;
	}


}
