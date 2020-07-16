// Copyright notice:
// 
// This example has been ported from Sasha Willems' "01 Image Processing" example,
// released under MIT license, and provided on GitHub:
// https://github.com/SaschaWillems/Vulkan#ComputeShader Copyright (c) 2016 Sascha Willems
//

#include <exekutor.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

class compute_image_processing_app : public xk::cg_element
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

public: // v== ak::cg_element overrides which will be invoked by the framework ==v
	compute_image_processing_app(ak::queue& aQueue)
		: mQueue{ &aQueue }
		, mRotationSpeed{ 1.0f }
	{}
	
	void initialize() override
	{
		// Create and upload vertex data for a quad
		mVertexBuffer = xk::context().create_buffer(
			ak::memory_usage::device, {},
			ak::vertex_buffer_meta::create_from_data(mVertexData)
		);
		mVertexBuffer->fill(
			mVertexData.data(), 0,
			ak::sync::wait_idle()
		);

		// Create and upload incides for drawing the quad
		mIndexBuffer = xk::context().create_buffer(
			ak::memory_usage::device, {},
			ak::index_buffer_meta::create_from_data(mIndices)
		);
		mIndexBuffer->fill(
			mIndices.data(), 0,
			ak::sync::wait_idle()
		);

		// Create a host-coherent buffer for the matrices
		mUbo = xk::context().create_buffer(
			ak::memory_usage::host_coherent, {},
			ak::uniform_buffer_meta::create_from_data(MatricesForUbo{})
		);

		// Load an image from file, upload it and create a view and a sampler for it
		mInputImageAndSampler = xk::context().create_image_sampler(
			xk::context().create_image_view(xk::create_image_from_file("assets/lion.png", false, false, 4, ak::memory_usage::device, ak::image_usage::general_storage_image)), // TODO: We could bind the image as a texture instead of a (readonly) storage image, then we would not need the "storage_image" usage type
			xk::context().create_sampler(ak::filter_mode::bilinear, ak::border_handling_mode::clamp_to_edge)
		);
		const auto wdth = mInputImageAndSampler->width();
		const auto hght = mInputImageAndSampler->height();
		const auto frmt = mInputImageAndSampler->format();

		// Create an image to write the modified result into, also create view and sampler for that
		mTargetImageAndSampler = xk::context().create_image_sampler(
			xk::context().create_image_view(
				xk::context().create_image( // Create an image and set some properties:
					wdth, hght, frmt, 1 /* one layer */, ak::memory_usage::device, /* in GPU memory */
					ak::image_usage::general_storage_image // This flag means (among other usages) that the image can be written to
				)
			),
			xk::context().create_sampler(ak::filter_mode::bilinear, ak::border_handling_mode::clamp_to_edge)
		);
		// Initialize the image with the contents of the input image:
		ak::copy_image_to_another(
			mInputImageAndSampler->get_image_view()->get_image(), 
			mTargetImageAndSampler->get_image_view()->get_image(), 
			ak::sync::wait_idle()
		);

		// Create our rasterization graphics pipeline with the required configuration:
		mGraphicsPipeline = xk::context().create_graphics_pipeline_for(
			ak::vertex_input_location(0, &Vertex::pos),
			ak::vertex_input_location(1, &Vertex::uv),
			"shaders/texture.vert",
			"shaders/texture.frag",
			ak::cfg::front_face::define_front_faces_to_be_clockwise(),
			ak::cfg::culling_mode::disabled,
			ak::cfg::viewport_depth_scissors_config::from_framebuffer(xk::context().main_window()->backbuffer_at_index(0)),
			ak::attachment::declare(xk::format_from_window_color_buffer(xk::context().main_window()), ak::on_load::clear, ak::color(0), ak::on_store::store).set_clear_color({0.f, 0.5f, 0.75f, 0.0f}),  // But not in presentable format, because ImGui comes after
			ak::binding(0, mUbo),
			ak::binding(1, mInputImageAndSampler) // Just take any image_sampler, as this is just used to describe the pipeline's layout.
		);

		// Create 3 compute pipelines:
		mComputePipelines.resize(3);
		mComputePipelines[0] = xk::context().create_compute_pipeline_for(
			"shaders/emboss.comp",
			ak::binding(0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			ak::binding(1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);
		mComputePipelines[1] = xk::context().create_compute_pipeline_for(
			"shaders/edgedetect.comp",
			ak::binding(0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			ak::binding(1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);
		mComputePipelines[2] = xk::context().create_compute_pipeline_for(
			"shaders/sharpen.comp",
			ak::binding(0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			ak::binding(1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);

		// Create a fence to ensure that the resources (via the mComputeDescriptorSet) are not used concurrently by concurrent compute shader executions
		mComputeFence = xk::context().create_fence(true); // Create in signaled state, because the first operation we'll call 

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = xk::context().create_descriptor_cache();
		
		// We are going to pre-record command buffers.
		// In this case: ONE PER BACKBUFFER, not per frame in flight!
		const auto w = xk::context().main_window()->swap_chain_extent().width;
		const auto halfW = w * 0.5f;
		const auto h = xk::context().main_window()->swap_chain_extent().height;
		const auto numBackbuffers = xk::context().main_window()->number_of_swapchain_images();
		for (size_t i = 0; i < numBackbuffers; ++i) {
			auto* mainWnd = xk::context().main_window();
			auto& commandPool = xk::context().get_command_pool_for_reusable_command_buffers(*mQueue);
		
			auto commandBuffer = commandPool->alloc_command_buffer();
			commandBuffer->begin_recording();
			
			// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
			commandBuffer->establish_image_memory_barrier(
				mTargetImageAndSampler->get_image_view()->get_image(),
				ak::pipeline_stage::compute_shader, 
				ak::pipeline_stage::fragment_shader,
				ak::memory_access::shader_buffers_and_images_write_access, 
				ak::memory_access::shader_buffers_and_images_read_access
			);

			// Prepare some stuff:
			auto vpLeft = vk::Viewport{0.0f, 0.0f, halfW, static_cast<float>(h)};
			auto vpRight = vk::Viewport{halfW, 0.0f, halfW, static_cast<float>(h)};

			// Begin drawing:
			commandBuffer->begin_render_pass_for_framebuffer(mGraphicsPipeline->get_renderpass(), mainWnd->backbuffer_at_index(i));
			commandBuffer->bind_pipeline(mGraphicsPipeline);

			// Draw left viewport:
			commandBuffer->handle().setViewport(0, 1, &vpLeft);

			commandBuffer->bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				ak::binding(0, mUbo),
				ak::binding(1, mInputImageAndSampler)
			}));
			commandBuffer->draw_indexed(*mIndexBuffer, *mVertexBuffer);

			// Draw right viewport (post compute)
			commandBuffer->handle().setViewport(0, 1, &vpRight);
			commandBuffer->bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				ak::binding(0, mUbo),
				ak::binding(1, mTargetImageAndSampler)
			}));
			commandBuffer->draw_indexed(*mIndexBuffer, *mVertexBuffer);

			commandBuffer->end_render_pass();
			commandBuffer->end_recording();
			mCmdBfrs.push_back(std::move(commandBuffer));
		}
		
		auto imguiManager = xk::current_composition()->element_by_type<xk::imgui_manager>();
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
		auto* mainWnd = xk::context().main_window();
		const auto w = mainWnd->swap_chain_extent().width;
		const auto h = mainWnd->swap_chain_extent().height;
		MatricesForUbo uboVS{};
		uboVS.projection = glm::perspective(glm::radians(60.0f), w * 0.5f / h, 0.1f, 256.0f);
		uboVS.model = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, -3.0));
		uboVS.model = uboVS.model * glm::rotate(glm::mat4{1.0f}, glm::radians(xk::time().time_since_start() * mRotationSpeed * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Update the buffer:
		mUbo->fill(&uboVS, 0, ak::sync::not_required());

		// Handle some input:
		if (xk::input().key_pressed(xk::key_code::num0)) {
			// [0] => Copy the input image to the target image and use a semaphore to sync the next draw call
			ak::copy_image_to_another(
				mInputImageAndSampler->get_image_view()->get_image(), 
				mTargetImageAndSampler->get_image_view()->get_image(),
				ak::sync::with_barriers(xk::context().main_window()->command_buffer_lifetime_handler())
			);
		}

		if (xk::input().key_down(xk::key_code::num1) || xk::input().key_pressed(xk::key_code::num2) || xk::input().key_pressed(xk::key_code::num3)) {
			// [1], [2], or [3] => Use a compute shader to modify the image

			// Use a fence to ensure that compute command buffer has finished execution before using it again
			mComputeFence->wait_until_signalled();
			mComputeFence->reset();

			size_t computeIndex = 0;
			if (xk::input().key_pressed(xk::key_code::num2)) { computeIndex = 1; }
			if (xk::input().key_pressed(xk::key_code::num3)) { computeIndex = 2; }

			// [1] => Apply the first compute shader on the input image
			auto& commandPool = xk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
			auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			cmdbfr->begin_recording();

			// Bind the pipeline

			// Set the descriptors of the uniform buffer
			cmdbfr->bind_pipeline(mComputePipelines[computeIndex]);
			cmdbfr->bind_descriptors(mComputePipelines[computeIndex]->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				ak::binding(0, mInputImageAndSampler->get_image_view()->as_storage_image()),
				ak::binding(1, mTargetImageAndSampler->get_image_view()->as_storage_image())
			}));
			cmdbfr->handle().dispatch(mInputImageAndSampler->width() / 16, mInputImageAndSampler->height() / 16, 1);

			cmdbfr->end_recording();
			
			auto submitInfo = vk::SubmitInfo()
				.setCommandBufferCount(1u)
				.setPCommandBuffers(cmdbfr->handle_ptr());

			mQueue->handle().submit({ submitInfo }, mComputeFence->handle());
			cmdbfr->invoke_post_execution_handler();

			mComputeFence->set_custom_deleter([
				ownedCmdbfr{ std::move(cmdbfr) } // Let the lifetime of this buffer be handled by the fence. the command buffer will be deleted at the next fence::reset
			](){});
		}

		if (xk::input().key_pressed(xk::key_code::escape)) {
			// Stop the current composition:
			xk::current_composition()->stop();
		}
	}

	void render() override
	{
		auto mainWnd = xk::context().main_window();
		const auto curIndex = mainWnd->current_image_index();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto& imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();

		// Submit one of the prepared command buffers:
		mQueue->submit(mCmdBfrs[curIndex], imageAvailableSemaphore);
	}

private: // v== Member variables ==v

	ak::queue* mQueue;
	ak::buffer mVertexBuffer;
	ak::buffer mIndexBuffer;
	ak::buffer mUbo;
	ak::image_sampler mInputImageAndSampler;
	ak::image_sampler mTargetImageAndSampler;
	ak::descriptor_cache mDescriptorCache;
	std::vector<ak::command_buffer> mCmdBfrs;

	ak::graphics_pipeline mGraphicsPipeline;

	std::vector<ak::compute_pipeline> mComputePipelines;
	ak::fence mComputeFence;

	float mRotationSpeed;
	
}; // compute_image_processing_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto mainWnd = xk::context().create_window("Compute Image Effects Example");
		mainWnd->set_resolution({ 1600, 800 });
		mainWnd->set_presentaton_mode(xk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = xk::context().create_queue({}, ak::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main ak::element which contains all the functionality:
		auto app = compute_image_processing_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = xk::imgui_manager(singleQueue);

		// GO:
		xk::execute(
			xk::application_name("Exekutor + Auto-Vk Example: Compute Image Effects Example"),
			mainWnd,
			app,
			ui
		);
	}
	catch (xk::logic_error&) {}
	catch (xk::runtime_error&) {}
	catch (ak::logic_error&) {}
	catch (ak::runtime_error&) {}
}
