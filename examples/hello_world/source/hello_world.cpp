#include <cg_base.hpp>

class draw_a_triangle_app : public cgb::cg_element
{
	/*void createGraphicsPipeline() {
        auto vertShaderCode = cgb::shader::create(cgb::shader_info::create("shaders/a_triangle.vert"));
        auto fragShaderCode = cgb::shader::create(cgb::shader_info::create("shaders/a_triangle.frag"));

        VkShaderModule vertShaderModule = vertShaderCode.handle();
        VkShaderModule fragShaderModule = fragShaderCode.handle();

        VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) 640;
        viewport.height = (float) 480;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = { 640, 480 };

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if (vkCreatePipelineLayout(cgb::context().logical_device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = cgb::context().main_window()->renderpass_handle();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(cgb::context().logical_device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

    }

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
*/

	void initialize() override
	{
		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		mPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_shader("shaders/a_triangle.vert"),
			cgb::fragment_shader("shaders/a_triangle.frag"),
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()),
			cgb::attachment::create_color(swapChainFormat)
		);

		mCmdBfrs = cgb::context().graphics_queue().pool().get_command_buffers(cgb::context().main_window()->number_of_concurrent_frames(), vk::CommandBufferUsageFlagBits::eSimultaneousUse);
		for (auto i = 0; i < mCmdBfrs.size(); ++i) { // TODO: WTF, this must be abstracted somehow!
			auto& cmdbfr = mCmdBfrs[i];
			cmdbfr.begin_recording();

			//auto renderPassHandle = cgb::get(mPipeline).renderpass_handle();
			auto renderPassHandle = cgb::context().main_window()->renderpass_handle();
			auto extent = cgb::context().main_window()->swap_chain_extent();

			cmdbfr.begin_render_pass(renderPassHandle, cgb::context().main_window()->backbuffer_at_index(i)->handle(), { 0, 0 }, extent);
			//cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipe.layout_handle(), 0u, { }, {});
			cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline->handle());
			cmdbfr.handle().draw(3u, 1u, 0u, 0u);
			//cgb::context().draw_triangle(cgb::get(mPipeline), cmdbfr);
			//cgb::context().draw_vertices(mPipeline, cmdbfr, mVertexBuffer);
			//cgb::context().draw_indexed(mPipeline, cmdbfr, mVertexBuffer, mIndexBuffer);
			//cgb::context().draw_indexed(mPipeline, cmdbfr, mModelVertices, mModelIndices);
			cmdbfr.end_render_pass();

			// TODO: image barriers instead of wait idle!!
			cgb::context().graphics_queue().handle().waitIdle();

			//cmdbfr.set_image_barrier(mOffscreenImages[i]->create_barrier(vk::AccessFlags(), vk::AccessFlagBits::eShaderWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral));

			//cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eRayTracingNV, mRtPipeline.mPipeline);
			//cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eRayTracingNV, mRtPipeline.mPipelineLayout, 0u, { mRtDescriptorSets[i].mDescriptorSet }, {});
			//cmdbfr.handle().traceRaysNV(
			//	mShaderBindingTable.mBuffer, 0,
			//	mShaderBindingTable.mBuffer, 3 * rtProps.shaderGroupHandleSize, rtProps.shaderGroupHandleSize,
			//	mShaderBindingTable.mBuffer, 1 * rtProps.shaderGroupHandleSize, rtProps.shaderGroupHandleSize,
			//	nullptr, 0, 0,
			//	extent.width, extent.height, 1,
			//	cgb::context().dynamic_dispatch());

			//cmdbfr.set_image_barrier(cgb::create_image_barrier(mSwapChainData->mSwapChainImages[i], mSwapChainData->mSwapChainImageFormat.mFormat, vk::AccessFlags(), vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal));
			//cmdbfr.set_image_barrier(mOffscreenImages[i]->create_barrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead, vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal));

			//cmdbfr.copy_image(*mOffscreenImages[i], mSwapChainData->mSwapChainImages[i]);

			//cmdbfr.set_image_barrier(cgb::create_image_barrier(mSwapChainData->mSwapChainImages[i], mSwapChainData->mSwapChainImageFormat.mFormat, vk::AccessFlagBits::eTransferWrite, vk::AccessFlags(), vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR));
	

			cmdbfr.end_recording();
		}
	}

	void render() override
	{
		auto bufferIndex = cgb::context().main_window()->image_index_for_frame();
		//auto& lol = cgb::context().main_window()->backbuffer_at_index(cgb::context().main_window()->image_index_for_frame(cgb::context().main_window()->current_frame()));
		//LOG_INFO(fmt::format("Current Frame's back buffer id: {}", fmt::ptr(&lol.handle())));
		cgb::context().main_window()->render_frame({ mCmdBfrs[bufferIndex] });
	}

	void update() override
	{
		if (cgb::input().key_pressed(cgb::key_code::h)) {
			// Log a message:
			LOG_INFO_EM("Hello cg_base!");
		}
		if (cgb::input().key_pressed(cgb::key_code::c)) {
			// Center the cursor:
			auto resolution = cgb::context().main_window()->resolution();
			cgb::context().main_window()->set_cursor_pos({ resolution[0] / 2.0, resolution[1] / 2.0 });
		}
		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// Stop the current composition:
			cgb::current_composition().stop();
		}
	}

private:
	cgb::graphics_pipeline mPipeline;
	std::vector<cgb::command_buffer> mCmdBfrs;
};

int main()
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Hello, World!";

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Hello World Window");
		mainWnd->set_resolution({ 640, 480 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->open(); 

		// Create an instance of my_first_cgb_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = draw_a_triangle_app();

		// Create a composition of:
		//  - a timer
		//  - an executor
		//  - a behavior
		// ...
		auto hello = cgb::composition<cgb::varying_update_timer, cgb::sequential_executor>({
				&element
			});

		// ... and start that composition!
		hello.start();
	}
	catch (std::runtime_error& re)
	{
		LOG_ERROR_EM(re.what());
	}
}


