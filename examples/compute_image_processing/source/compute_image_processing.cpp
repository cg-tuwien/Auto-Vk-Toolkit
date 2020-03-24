#include <cg_base.hpp>

class compute_image_processing_app : public cgb::cg_element
{
private: // v== Struct definitions and data ==v

	// Define a struct for our vertex input data:
	struct Vertex {
	    glm::vec3 pos;
	    glm::vec2 uv;
	};

	// Define a struct for the uniform buffer which holds the
	// transformation matrices, used in the vertex shader.
	struct MatricesForUbo {
		glm::mat4 projection;
		glm::mat4 model;
	};

	// Vertex data for a quad:
	const std::vector<Vertex> mVertexData = {
		{{  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f }},
		{{ -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f }},
		{{ -1.0f, -1.0f, 0.0f }, { 0.0f, 0.0f }},
		{{  1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f }},
	};

	// Indices for the quad:
	const std::vector<uint16_t> mIndices = {
		 0, 1, 2,   2, 3, 0
	};

public: // v== cgb::cg_element overrides which will be invoked by the framework ==v

	void initialize() override
	{
		// Create and upload vertex data for a quad
		mVertexBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(mVertexData), 
			cgb::memory_usage::device,
			mVertexData.data(),
			cgb::sync::with_barriers_on_current_frame()
		);

		// Create and upload incides for drawing the quad
		mIndexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(mIndices),	
			cgb::memory_usage::device,
			mIndices.data(),
			cgb::sync::with_barriers_on_current_frame()
		);

		// Create a host-coherent buffer for the matrices
		mUbo = cgb::create(
			cgb::uniform_buffer_meta::create_from_data(MatricesForUbo{}),
			cgb::memory_usage::host_coherent
		);

		// Load an image from file, upload it and create a view and a sampler for it
		mInputImageAndSampler = cgb::image_sampler_t::create(
			cgb::image_view_t::create(cgb::create_image_from_file("assets/lion.png")),
			cgb::sampler_t::create(cgb::filter_mode::bilinear, cgb::border_handling_mode::clamp_to_edge)
		);
		auto wdth = mInputImageAndSampler->width();
		auto hght = mInputImageAndSampler->height();
		auto frmt = mInputImageAndSampler->format();

		// Create an image to write the modified result into, also create view and sampler for that
		mTargetImageAndSampler = cgb::image_sampler_t::create(
			cgb::image_view_t::create(
				cgb::image_t::create( // Create an image and set some properties:
					wdth, hght, frmt, false /* no mip-maps */, 1 /* one layer */, cgb::memory_usage::device, /* in GPU memory */
					cgb::image_usage::versatile_image // This flag means (among other usages) that the image can be written to
				)
			),
			cgb::sampler_t::create(cgb::filter_mode::bilinear, cgb::border_handling_mode::clamp_to_edge)
		);
		cgb::transition_image_layout(
			mTargetImageAndSampler->get_image_view()->get_image(), 
			mTargetImageAndSampler->get_image_view()->get_image().format().mFormat, 
			vk::ImageLayout::eGeneral); // TODO: This must be abstracted!
		// Initialize the image with the contents of the input image:
		cgb::copy_image_to_another(mInputImageAndSampler->get_image_view()->get_image(), mTargetImageAndSampler->get_image_view()->get_image(), 
			[](cgb::semaphore _CopyCompleteSemaphore) {
				cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_CopyCompleteSemaphore));
			});

		auto swapChainFormat = cgb::context().main_window()->swap_chain_image_format();
		// Create our rasterization graphics pipeline with the required configuration:
		mGraphicsPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_input_location(0, &Vertex::pos),
			cgb::vertex_input_location(1, &Vertex::uv),
			"shaders/texture.vert",
			"shaders/texture.frag",
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::culling_mode::disabled,
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()).enable_dynamic_viewport(),
			cgb::attachment::create_color(swapChainFormat),
			cgb::binding(0, mUbo),
			cgb::binding(1, mInputImageAndSampler) // Just take any image_sampler, as this is just used to describe the pipeline's layout.
		);

		// The following is a bit ugly and needs to be abstracted sometime in the future. 
		// Right now it is neccessary to upload the resource descriptors to the GPU (the information about the UBO and the samplers, in particular).
		// These descriptor set will be used in render(). It is only created once to save memory/to make lifetime management easier.
		mDescriptorSetPreCompute.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		mDescriptorSetPostCompute.reserve(cgb::context().main_window()->number_of_in_flight_frames());
		for (int i = 0; i < cgb::context().main_window()->number_of_in_flight_frames(); ++i) {
			// We'll need different descriptor sets for the left view and for the right view, since we are using different resources
			// 1. left view, a.k.a. pre compute
			mDescriptorSetPreCompute.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSetPreCompute.back() = cgb::descriptor_set::create({ 
				cgb::binding(0, mUbo),
				cgb::binding(1, mInputImageAndSampler)
			});	
			// 2. right view, a.k.a. post compute
			mDescriptorSetPostCompute.emplace_back(std::make_shared<cgb::descriptor_set>());
			*mDescriptorSetPostCompute.back() = cgb::descriptor_set::create({ 
				cgb::binding(0, mUbo),
				cgb::binding(1, mTargetImageAndSampler)
			});		
		}

		// Create 3 compute pipelines:
		mComputePipelines.resize(3);
		mComputePipelines[0] = cgb::compute_pipeline_for(
			"shaders/emboss.comp",
			cgb::binding(0, mInputImageAndSampler->get_image_view()),
			cgb::binding(1, mTargetImageAndSampler->get_image_view())
		);
		mComputePipelines[1] = cgb::compute_pipeline_for(
			"shaders/edgedetect.comp",
			cgb::binding(0, mInputImageAndSampler->get_image_view()),
			cgb::binding(1, mTargetImageAndSampler->get_image_view())
		);
		mComputePipelines[2] = cgb::compute_pipeline_for(
			"shaders/sharpen.comp",
			cgb::binding(0, mInputImageAndSampler->get_image_view()),
			cgb::binding(1, mTargetImageAndSampler->get_image_view())
		);

		// Create one descriptor set which will be used for all the compute pipelines:
		mComputeDescriptorSet = std::make_shared<cgb::descriptor_set>();
		*mComputeDescriptorSet = cgb::descriptor_set::create({ 
			cgb::binding(0, mInputImageAndSampler->get_image_view()),
			cgb::binding(1, mTargetImageAndSampler->get_image_view())
		});	

		// Create a fence to ensure that the resources (via the mComputeDescriptorSet) are not used concurrently by concurrent compute shader executions
		mComputeFence = cgb::fence_t::create(true); // Create in signaled state, because the first operation we'll call 

		// Record render command buffers - one for each frame in flight:
		auto w = cgb::context().main_window()->swap_chain_extent().width;
		auto halfW = w * 0.5f;
		auto h = cgb::context().main_window()->swap_chain_extent().height;
		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), [&](cgb::command_buffer& _CommandBuffer, int64_t _InFlightIndex) {
			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			auto barrier = vk::ImageMemoryBarrier{}
				.setOldLayout(vk::ImageLayout::eGeneral)
				.setNewLayout(vk::ImageLayout::eGeneral)
				.setImage(mTargetImageAndSampler->image_handle())
				.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0u, 1u, 0u, 1u })
				.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
				.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
			_CommandBuffer->handle().pipelineBarrier(
				vk::PipelineStageFlagBits::eComputeShader,
				vk::PipelineStageFlagBits::eFragmentShader,
				{},
				0, nullptr,
				0, nullptr,
				1, &barrier);

			// Prepare some stuff:
			auto scissor = vk::Rect2D{}.setOffset({0, 0}).setExtent({w, h});
			auto vpLeft = vk::Viewport{0.0f, 0.0f, halfW, static_cast<float>(h)};
			auto vpRight = vk::Viewport{halfW, 0.0f, halfW, static_cast<float>(h)};

			// Begin drawing:
			_CommandBuffer.begin_render_pass_for_window(cgb::context().main_window(), _InFlightIndex);

			// Draw left viewport (pre compute)
			_CommandBuffer.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->layout_handle(), 0, 
				mDescriptorSetPreCompute[cgb::context().main_window()->in_flight_index_for_frame()]->number_of_descriptor_sets(),
				mDescriptorSetPreCompute[cgb::context().main_window()->in_flight_index_for_frame()]->descriptor_sets_addr(), 
				0, nullptr);
			_CommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->handle());
			_CommandBuffer.handle().setViewport(0, 1, &vpLeft);
			cgb::context().draw_indexed(mGraphicsPipeline, _CommandBuffer, mVertexBuffer, mIndexBuffer);

			// Draw right viewport (post compute)
			_CommandBuffer.handle().bindDescriptorSets(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->layout_handle(), 0, 
				mDescriptorSetPostCompute[cgb::context().main_window()->in_flight_index_for_frame()]->number_of_descriptor_sets(),
				mDescriptorSetPostCompute[cgb::context().main_window()->in_flight_index_for_frame()]->descriptor_sets_addr(), 
				0, nullptr);
			_CommandBuffer.handle().bindPipeline(vk::PipelineBindPoint::eGraphics, mGraphicsPipeline->handle());
			_CommandBuffer.handle().setViewport(0, 1, &vpRight);
			cgb::context().draw_indexed(mGraphicsPipeline, _CommandBuffer, mVertexBuffer, mIndexBuffer);

			_CommandBuffer.end_render_pass();
		});
	}

	void render() override
	{
		auto curIndex = cgb::context().main_window()->in_flight_index_for_frame();
		submit_command_buffer_ref(mCmdBfrs[curIndex]);
	}

	void update() override
	{
		// Update the UBO's data:
		auto w = cgb::context().main_window()->swap_chain_extent().width;
		auto h = cgb::context().main_window()->swap_chain_extent().height;
		auto uboVS = MatricesForUbo{};
		uboVS.projection = glm::perspective(glm::radians(60.0f), w * 0.5f / h, 0.1f, 256.0f);
		uboVS.model = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, -3.0));
		uboVS.model = uboVS.model * glm::rotate(glm::mat4{1.0f}, glm::radians(cgb::time().time_since_start() * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Update the buffer:
		cgb::fill(mUbo, &uboVS);

		// Handle some input:
		if (cgb::input().key_pressed(cgb::key_code::num0)) {
			// [0] => Copy the input image to the target image and use a semaphore to sync the next draw call
			cgb::copy_image_to_another(
				mInputImageAndSampler->get_image_view()->get_image(), 
				mTargetImageAndSampler->get_image_view()->get_image(),
				[](cgb::semaphore _CopyCompleteSemaphore) {
					cgb::context().main_window()->set_extra_semaphore_dependency(std::move(_CopyCompleteSemaphore));
				});
		}

		if (cgb::input().key_down(cgb::key_code::num1) || cgb::input().key_pressed(cgb::key_code::num2) || cgb::input().key_pressed(cgb::key_code::num3)) {
			// [1], [2], or [3] => Use a compute shader to modify the image

			// Use a fence to ensure that compute command buffer has finished executin before using it again
			mComputeFence->wait_until_signalled();
			mComputeFence->reset();

			size_t computeIndex = 0;
			if (cgb::input().key_pressed(cgb::key_code::num2)) { computeIndex = 1; }
			if (cgb::input().key_pressed(cgb::key_code::num3)) { computeIndex = 2; }

			// [1] => Apply the first compute shader on the input image
			auto cmdbfr = cgb::context().graphics_queue().pool().get_command_buffer(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
			cmdbfr.begin_recording();

			// Bind the pipeline

			// Set the descriptors of the uniform buffer
			cmdbfr.handle().bindDescriptorSets(vk::PipelineBindPoint::eCompute, mComputePipelines[computeIndex]->layout_handle(), 0, 
				mComputeDescriptorSet->number_of_descriptor_sets(),
				mComputeDescriptorSet->descriptor_sets_addr(), 
				0, nullptr);
			cmdbfr.handle().bindPipeline(vk::PipelineBindPoint::eCompute, mComputePipelines[computeIndex]->handle());
			cmdbfr.handle().dispatch(mInputImageAndSampler->width() / 16, mInputImageAndSampler->height() / 16, 1);

			cmdbfr.end_recording();
			
			vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands; // Just set to all commands. Don't know if this could be optimized somehow?!
			auto submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1u)
				.setPCommandBuffers(cmdbfr.handle_addr());

			cgb::context().graphics_queue().handle().submit({ submitInfo }, mComputeFence->handle());

			mComputeFence->set_custom_deleter([
				ownedCmdbfr{ std::move(cmdbfr) }
			](){});
		}

		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// Stop the current composition:
			cgb::current_composition().stop();
		}
	}

private: // v== Member variables ==v

	cgb::vertex_buffer mVertexBuffer;
	cgb::index_buffer mIndexBuffer;
	cgb::uniform_buffer mUbo;
	cgb::image_sampler mInputImageAndSampler;
	cgb::image_sampler mTargetImageAndSampler;
	std::vector<cgb::command_buffer> mCmdBfrs;

	cgb::graphics_pipeline mGraphicsPipeline;
	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSetPreCompute;
	std::vector<std::shared_ptr<cgb::descriptor_set>> mDescriptorSetPostCompute;

	std::vector<cgb::compute_pipeline> mComputePipelines;
	std::shared_ptr<cgb::descriptor_set> mComputeDescriptorSet;
	cgb::fence mComputeFence;

}; // compute_image_processing_app

int main() // <== Starting point ==
{
	try {
		// What's the name of our application
		cgb::settings::gApplicationName = "Compute Image Effects Example";
		cgb::settings::gQueueSelectionPreference = cgb::device_queue_selection_strategy::prefer_everything_on_single_queue;

		// Create a window and open it
		auto mainWnd = cgb::context().create_window("Compute Image Effects");
		mainWnd->set_resolution({ 1280, 800 });
		mainWnd->set_presentaton_mode(cgb::presentation_mode::vsync);
		mainWnd->open(); 

		// Create an instance of compute_image_processing_app which, in this case,
		// contains the entire functionality of our application. 
		auto element = compute_image_processing_app();

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
