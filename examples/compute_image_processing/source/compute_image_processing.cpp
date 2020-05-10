// Copyright notice:
// 
// This example has been ported from Sasha Willems' "01 Image Processing" example,
// released under MIT license, and provided on GitHub:
// https://github.com/SaschaWillems/Vulkan#ComputeShader Copyright (c) 2016 Sascha Willems
//

#include <cg_base.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

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
	compute_image_processing_app() : mRotationSpeed{ 1.0f } {}
	
	void initialize() override
	{
		// Create and upload vertex data for a quad
		mVertexBuffer = cgb::create_and_fill(
			cgb::vertex_buffer_meta::create_from_data(mVertexData), 
			cgb::memory_usage::device,
			mVertexData.data(),
			cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler())
		);

		// Create and upload incides for drawing the quad
		mIndexBuffer = cgb::create_and_fill(
			cgb::index_buffer_meta::create_from_data(mIndices),	
			cgb::memory_usage::device,
			mIndices.data(),
			cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler())
		);

		// Create a host-coherent buffer for the matrices
		mUbo = cgb::create(
			cgb::uniform_buffer_meta::create_from_data(MatricesForUbo{}),
			cgb::memory_usage::host_coherent
		);

		// Load an image from file, upload it and create a view and a sampler for it
		mInputImageAndSampler = cgb::image_sampler_t::create(
			cgb::image_view_t::create(cgb::create_image_from_file("assets/lion.png", false, true, 4, cgb::memory_usage::device, cgb::image_usage::general_storage_image)), // TODO: Is it not possible to use this image without the "storage" flag given that it is used as a "readonly image" in the compute shader only?
			cgb::sampler_t::create(cgb::filter_mode::bilinear, cgb::border_handling_mode::clamp_to_edge)
		);
		const auto wdth = mInputImageAndSampler->width();
		const auto hght = mInputImageAndSampler->height();
		const auto frmt = mInputImageAndSampler->format();

		// Create an image to write the modified result into, also create view and sampler for that
		mTargetImageAndSampler = cgb::image_sampler_t::create(
			cgb::image_view_t::create(
				cgb::image_t::create( // Create an image and set some properties:
					wdth, hght, frmt, 1 /* one layer */, cgb::memory_usage::device, /* in GPU memory */
					cgb::image_usage::general_storage_image // This flag means (among other usages) that the image can be written to
				)
			),
			cgb::sampler_t::create(cgb::filter_mode::bilinear, cgb::border_handling_mode::clamp_to_edge)
		);
		// Initialize the image with the contents of the input image:
		cgb::copy_image_to_another(
			mInputImageAndSampler->get_image_view()->get_image(), 
			mTargetImageAndSampler->get_image_view()->get_image(), 
			cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler())
		);

		using namespace cgb::att;
		
		// Create our rasterization graphics pipeline with the required configuration:
		mGraphicsPipeline = cgb::graphics_pipeline_for(
			cgb::vertex_input_location(0, &Vertex::pos),
			cgb::vertex_input_location(1, &Vertex::uv),
			"shaders/texture.vert",
			"shaders/texture.frag",
			cgb::cfg::front_face::define_front_faces_to_be_clockwise(),
			cgb::cfg::culling_mode::disabled,
			cgb::cfg::viewport_depth_scissors_config::from_window(cgb::context().main_window()).enable_dynamic_viewport(),
			cgb::attachment::declare(cgb::image_format::from_window_color_buffer(), on_load::clear, color(0), on_store::store).set_clear_color({0.f, 0.5f, 0.75f, 0.0f}),  // But not in presentable format, because ImGui comes after
			cgb::binding(0, mUbo),
			cgb::binding(1, mInputImageAndSampler) // Just take any image_sampler, as this is just used to describe the pipeline's layout.
		);

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

		// Create a fence to ensure that the resources (via the mComputeDescriptorSet) are not used concurrently by concurrent compute shader executions
		mComputeFence = cgb::fence_t::create(true); // Create in signaled state, because the first operation we'll call 

		// Record render command buffers - one for each frame in flight:
		auto w = cgb::context().main_window()->swap_chain_extent().width;
		auto halfW = w * 0.5f;
		auto h = cgb::context().main_window()->swap_chain_extent().height;
		mCmdBfrs = record_command_buffers_for_all_in_flight_frames(cgb::context().main_window(), cgb::context().graphics_queue(), [&](cgb::command_buffer_t& commandBuffer, int64_t inFlightIndex) {

			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			commandBuffer.establish_image_memory_barrier_rw(
				mTargetImageAndSampler->get_image_view()->get_image(),
				cgb::pipeline_stage::compute_shader, 
				cgb::pipeline_stage::fragment_shader,
				std::optional<cgb::write_memory_access>{cgb::memory_access::shader_buffers_and_images_write_access}, 
				std::optional<cgb::read_memory_access>{cgb::memory_access::shader_buffers_and_images_read_access}
			);

			// Prepare some stuff:
			auto scissor = vk::Rect2D{}.setOffset({0, 0}).setExtent({w, h});
			auto vpLeft = vk::Viewport{0.0f, 0.0f, halfW, static_cast<float>(h)};
			auto vpRight = vk::Viewport{halfW, 0.0f, halfW, static_cast<float>(h)};

			// Begin drawing:
			commandBuffer.begin_render_pass_for_framebuffer(mGraphicsPipeline->get_renderpass(), cgb::context().main_window()->backbufer_for_frame(inFlightIndex)); // TODO: only works because #concurrent == #present
			commandBuffer.bind_pipeline(mGraphicsPipeline);

			// Draw left viewport:
			commandBuffer.handle().setViewport(0, 1, &vpLeft);
			commandBuffer.bind_descriptors(mGraphicsPipeline->layout(), {
				cgb::binding(0, mUbo),
				cgb::binding(1, mInputImageAndSampler)
			});
			commandBuffer.draw_indexed(*mIndexBuffer, *mVertexBuffer);

			// Draw right viewport (post compute)
			commandBuffer.handle().setViewport(0, 1, &vpRight);
			commandBuffer.bind_descriptors(mGraphicsPipeline->layout(), {
				cgb::binding(0, mUbo),
				cgb::binding(1, mTargetImageAndSampler)
			});
			commandBuffer.draw_indexed(*mIndexBuffer, *mVertexBuffer);

			commandBuffer.end_render_pass();
		});

		auto imguiManager = cgb::current_composition().element_by_type<cgb::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::InputFloat("Rotation Speed", &mRotationSpeed, 0.1f, 1.0f);
				
				ImGui::Separator();

				static ImTextureID inputTexId = ImGui_ImplVulkan_AddTexture(mInputImageAndSampler->sampler_handle(), mInputImageAndSampler->view_handle(), VK_IMAGE_LAYOUT_GENERAL);
				// This ImTextureID-stuff is tough stuff -> see https://github.com/ocornut/imgui/pull/914
		        float inputTexWidth  = (float)mInputImageAndSampler->get_image_view()->get_image().config().extent.width;
		        float inputTexHeight = (float)mInputImageAndSampler->get_image_view()->get_image().config().extent.height;
				ImGui::Text("Input image (%.0fx%.0f):", inputTexWidth, inputTexHeight);
				ImGui::Image(inputTexId, ImVec2(inputTexWidth/6.0f, inputTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				static ImTextureID targetTexId = ImGui_ImplVulkan_AddTexture(mTargetImageAndSampler->sampler_handle(), mTargetImageAndSampler->view_handle(), VK_IMAGE_LAYOUT_GENERAL);
		        float targetTexWidth  = (float)mTargetImageAndSampler->get_image_view()->get_image().config().extent.width;
		        float targetTexHeight = (float)mTargetImageAndSampler->get_image_view()->get_image().config().extent.height;
				ImGui::Text("Output image (%.0fx%.0f):", targetTexWidth, targetTexHeight);
				ImGui::Image(targetTexId, ImVec2(targetTexWidth/6.0f, targetTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				ImGui::End();
			});
		}
	}

	void update() override
	{
		// Update the UBO's data:
		auto w = cgb::context().main_window()->swap_chain_extent().width;
		auto h = cgb::context().main_window()->swap_chain_extent().height;
		auto uboVS = MatricesForUbo{};
		uboVS.projection = glm::perspective(glm::radians(60.0f), w * 0.5f / h, 0.1f, 256.0f);
		uboVS.model = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, -3.0));
		uboVS.model = uboVS.model * glm::rotate(glm::mat4{1.0f}, glm::radians(cgb::time().time_since_start() * mRotationSpeed * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Update the buffer:
		cgb::fill(mUbo, &uboVS);

		// Handle some input:
		if (cgb::input().key_pressed(cgb::key_code::num0)) {
			// [0] => Copy the input image to the target image and use a semaphore to sync the next draw call
			cgb::copy_image_to_another(
				mInputImageAndSampler->get_image_view()->get_image(), 
				mTargetImageAndSampler->get_image_view()->get_image(),
				cgb::sync::with_barriers(cgb::context().main_window()->command_buffer_lifetime_handler())
			);
		}

		if (cgb::input().key_down(cgb::key_code::num1) || cgb::input().key_pressed(cgb::key_code::num2) || cgb::input().key_pressed(cgb::key_code::num3)) {
			// [1], [2], or [3] => Use a compute shader to modify the image

			// Use a fence to ensure that compute command buffer has finished execution before using it again
			mComputeFence->wait_until_signalled();
			mComputeFence->reset();

			size_t computeIndex = 0;
			if (cgb::input().key_pressed(cgb::key_code::num2)) { computeIndex = 1; }
			if (cgb::input().key_pressed(cgb::key_code::num3)) { computeIndex = 2; }

			// [1] => Apply the first compute shader on the input image
			auto cmdbfr = cgb::command_pool::create_single_use_command_buffer(cgb::context().graphics_queue());
			cmdbfr->begin_recording();

			// Bind the pipeline

			// Set the descriptors of the uniform buffer
			cmdbfr->bind_pipeline(mComputePipelines[computeIndex]);
			cmdbfr->bind_descriptors(mComputePipelines[computeIndex]->layout(), {
				cgb::binding(0, mInputImageAndSampler->get_image_view()),
				cgb::binding(1, mTargetImageAndSampler->get_image_view())
			});
			cmdbfr->handle().dispatch(mInputImageAndSampler->width() / 16, mInputImageAndSampler->height() / 16, 1);

			cmdbfr->end_recording();
			
			vk::PipelineStageFlags waitMask = vk::PipelineStageFlagBits::eAllCommands; // Just set to all commands. Don't know if this could be optimized somehow?!
			auto submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1u)
				.setPCommandBuffers(cmdbfr->handle_ptr());

			cgb::context().graphics_queue().handle().submit({ submitInfo }, mComputeFence->handle());
			cmdbfr->invoke_post_execution_handler();

			mComputeFence->set_custom_deleter([
				ownedCmdbfr{ std::move(cmdbfr) }
			](){});
		}

		if (cgb::input().key_pressed(cgb::key_code::escape)) {
			// Stop the current composition:
			cgb::current_composition().stop();
		}
	}

	void render() override
	{
		auto mainWnd = cgb::context().main_window();
		auto curIndex = mainWnd->in_flight_index_for_frame();
		mainWnd->submit_for_backbuffer_ref(mCmdBfrs[curIndex]);
	}

private: // v== Member variables ==v

	cgb::vertex_buffer mVertexBuffer;
	cgb::index_buffer mIndexBuffer;
	cgb::uniform_buffer mUbo;
	cgb::image_sampler mInputImageAndSampler;
	cgb::image_sampler mTargetImageAndSampler;
	std::vector<cgb::command_buffer> mCmdBfrs;

	cgb::graphics_pipeline mGraphicsPipeline;

	std::vector<cgb::compute_pipeline> mComputePipelines;
	cgb::fence mComputeFence;

	float mRotationSpeed;
	
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
		mainWnd->set_number_of_concurrent_frames(3);
		mainWnd->set_presentaton_mode(cgb::presentation_mode::fifo);
		mainWnd->open(); 

		// Create an instance of our main cgb::element which contains all the functionality:
		auto app = compute_image_processing_app();
		// Create another element for drawing the UI with ImGui
		auto ui = cgb::imgui_manager();

		// Compose our elements and engage:
		auto imageProcessing = cgb::setup(app, ui);
		imageProcessing.start();
	}
	catch (cgb::logic_error&) {}
	catch (cgb::runtime_error&) {}
}
