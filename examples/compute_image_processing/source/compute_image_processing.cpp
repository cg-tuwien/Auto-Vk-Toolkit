// Copyright notice:
// 
// This example has been ported from Sasha Willems' "01 Image Processing" example,
// released under MIT license, and provided on GitHub:
// https://github.com/SaschaWillems/Vulkan#ComputeShader Copyright (c) 2016 Sascha Willems
//

#include <gvk.hpp>
#include <imgui.h>
#include <imgui_impl_vulkan.h>

class compute_image_processing_app : public gvk::invokee
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

public: // v== avk::invokee overrides which will be invoked by the framework ==v
	compute_image_processing_app(avk::queue& aQueue)
		: mQueue{ &aQueue }
		, mRotationSpeed{ 1.0f }
	{}
	
	void initialize() override
	{
		// Create and upload vertex data for a quad
		mVertexBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::vertex_buffer_meta::create_from_data(mVertexData)
		);
		mVertexBuffer->fill(
			mVertexData.data(), 0,
			avk::sync::wait_idle()
		);

		// Create and upload incides for drawing the quad
		mIndexBuffer = gvk::context().create_buffer(
			avk::memory_usage::device, {},
			avk::index_buffer_meta::create_from_data(mIndices)
		);
		mIndexBuffer->fill(
			mIndices.data(), 0,
			avk::sync::wait_idle()
		);

		// Create a host-coherent buffer for the matrices
		auto fif = gvk::context().main_window()->number_of_frames_in_flight();
		for (decltype(fif) i = 0; i < fif; ++i) {
			mUbo.emplace_back(gvk::context().create_buffer(
				avk::memory_usage::host_visible, {},
				avk::uniform_buffer_meta::create_from_data(MatricesForUbo{})
			));
		}

		// Load an image from file, upload it and create a view and a sampler for it
		mInputImageAndSampler = gvk::context().create_image_sampler(
			gvk::context().create_image_view(gvk::create_image_from_file("assets/lion.png", false, false, true, 4, avk::memory_usage::device, avk::image_usage::general_storage_image)), // TODO: We could bind the image as a texture instead of a (readonly) storage image, then we would not need the "storage_image" usage type
			gvk::context().create_sampler(avk::filter_mode::bilinear, avk::border_handling_mode::clamp_to_edge)
		);
		const auto wdth = mInputImageAndSampler->width();
		const auto hght = mInputImageAndSampler->height();
		const auto frmt = mInputImageAndSampler->format();

		// Create an image to write the modified result into, also create view and sampler for that
		mTargetImageAndSampler = gvk::context().create_image_sampler(
			gvk::context().create_image_view(
				gvk::context().create_image( // Create an image and set some properties:
					wdth, hght, frmt, 1 /* one layer */, avk::memory_usage::device, /* in GPU memory */
					avk::image_usage::general_storage_image // This flag means (among other usages) that the image can be written to
				)
			),
			gvk::context().create_sampler(avk::filter_mode::bilinear, avk::border_handling_mode::clamp_to_edge)
		);
		// Initialize the image with the contents of the input image:
		avk::copy_image_to_another(
			mInputImageAndSampler->get_image(), 
			mTargetImageAndSampler->get_image(), 
			avk::sync::wait_idle()
		);

		// Create our rasterization graphics pipeline with the required configuration:
		mGraphicsPipeline = gvk::context().create_graphics_pipeline_for(
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::pos) -> to_location(0),
			avk::from_buffer_binding(0) -> stream_per_vertex(&Vertex::uv)  -> to_location(1),
			"shaders/texture.vert",
			"shaders/texture.frag",
			avk::cfg::front_face::define_front_faces_to_be_clockwise(),
			avk::cfg::culling_mode::disabled,
			avk::cfg::viewport_depth_scissors_config::from_framebuffer(gvk::context().main_window()->backbuffer_at_index(0)).enable_dynamic_viewport(),
			avk::attachment::declare(gvk::format_from_window_color_buffer(gvk::context().main_window()), avk::on_load::clear, avk::color(0), avk::on_store::store).set_clear_color({0.f, 0.5f, 0.75f, 0.0f}),  // But not in presentable format, because ImGui comes after
			avk::descriptor_binding(0, 0, mUbo[0]),	             // Just take any UBO, as this is just used to describe the pipeline's layout.
			avk::descriptor_binding(0, 1, mInputImageAndSampler) 
		);

		// Create 3 compute pipelines:
		mComputePipelines.resize(3);
		mComputePipelines[0] = gvk::context().create_compute_pipeline_for(
			"shaders/emboss.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);
		mComputePipelines[1] = gvk::context().create_compute_pipeline_for(
			"shaders/edgedetect.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);
		mComputePipelines[2] = gvk::context().create_compute_pipeline_for(
			"shaders/sharpen.comp",
			avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image()),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image())
		);

		mComputePipelines[0].enable_shared_ownership(); // Make it usable with the updater
		mComputePipelines[1].enable_shared_ownership(); // Make it usable with the updater
		mComputePipelines[2].enable_shared_ownership(); // Make it usable with the updater

		mUpdater.emplace();
		mUpdater->on(gvk::shader_files_changed_event(mComputePipelines[0])).update(mComputePipelines[0]);
		mUpdater->on(gvk::shader_files_changed_event(mComputePipelines[1])).update(mComputePipelines[1]);
		mUpdater->on(gvk::shader_files_changed_event(mComputePipelines[2])).update(mComputePipelines[2]);

		// Create a fence to ensure that the resources (via the mComputeDescriptorSet) are not used concurrently by concurrent compute shader executions
		mComputeFence = gvk::context().create_fence(true); // Create in signaled state, because the first operation we'll call 

		// Create a descriptor cache that helps us to conveniently create descriptor sets:
		mDescriptorCache = gvk::context().create_descriptor_cache();
		
		auto* imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
		if(nullptr != imguiManager) {
			imguiManager->add_callback([this, imguiManager](){
		        ImGui::Begin("Info & Settings");
				ImGui::SetWindowPos(ImVec2(1.0f, 1.0f), ImGuiCond_FirstUseEver);
				ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
				ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
				ImGui::InputFloat("Rotation Speed", &mRotationSpeed, 0.1f, 1.0f);
				
				ImGui::Separator();

				ImTextureID inputTexId = imguiManager->get_or_create_texture_descriptor(avk::referenced(mInputImageAndSampler));
		        auto inputTexWidth  = static_cast<float>(mInputImageAndSampler->get_image_view()->get_image().config().extent.width);
		        auto inputTexHeight = static_cast<float>(mInputImageAndSampler->get_image_view()->get_image().config().extent.height);
				ImGui::Text("Input image (%.0fx%.0f):", inputTexWidth, inputTexHeight);
				ImGui::Image(inputTexId, ImVec2(inputTexWidth/6.0f, inputTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				ImTextureID targetTexId = imguiManager->get_or_create_texture_descriptor(avk::referenced(mTargetImageAndSampler));
		        auto targetTexWidth  = static_cast<float>(mTargetImageAndSampler->get_image_view()->get_image().config().extent.width);
		        auto targetTexHeight = static_cast<float>(mTargetImageAndSampler->get_image_view()->get_image().config().extent.height);
				ImGui::Text("Output image (%.0fx%.0f):", targetTexWidth, targetTexHeight);
				ImGui::Image(targetTexId, ImVec2(targetTexWidth/6.0f, targetTexHeight/6.0f), ImVec2(0,0), ImVec2(1,1), ImVec4(1.0f,1.0f,1.0f,1.0f), ImVec4(1.0f,1.0f,1.0f,0.5f));

				ImGui::End();
			});
		}
	}

	void update() override
	{
		// Handle some input:
		if (gvk::input().key_pressed(gvk::key_code::num0)) {
			// [0] => Copy the input image to the target image and use a semaphore to sync the next draw call
			avk::copy_image_to_another(
				mInputImageAndSampler->get_image(), 
				mTargetImageAndSampler->get_image(),
				avk::sync::with_barriers(gvk::context().main_window()->command_buffer_lifetime_handler())
			);
		}

		if (gvk::input().key_down(gvk::key_code::num1) || gvk::input().key_pressed(gvk::key_code::num2) || gvk::input().key_pressed(gvk::key_code::num3)) {
			// [1], [2], or [3] => Use a compute shader to modify the image

			// Use a fence to ensure that compute command buffer has finished execution before using it again
			mComputeFence->wait_until_signalled();
			mComputeFence->reset();

			size_t computeIndex = 0;
			if (gvk::input().key_pressed(gvk::key_code::num2)) { computeIndex = 1; }
			if (gvk::input().key_pressed(gvk::key_code::num3)) { computeIndex = 2; }

			// [1] => Apply the first compute shader on the input image
			auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
			auto cmdbfr = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
			cmdbfr->begin_recording();

			// Bind the pipeline

			// Set the descriptors of the uniform buffer
			cmdbfr->bind_pipeline(avk::const_referenced(mComputePipelines[computeIndex]));
			cmdbfr->bind_descriptors(mComputePipelines[computeIndex]->layout(), mDescriptorCache.get_or_create_descriptor_sets({
				avk::descriptor_binding(0, 0, mInputImageAndSampler->get_image_view()->as_storage_image()),
				avk::descriptor_binding(0, 1, mTargetImageAndSampler->get_image_view()->as_storage_image())
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

		if (gvk::input().key_pressed(gvk::key_code::escape)) {
			// Stop the current composition:
			gvk::current_composition()->stop();
		}
	}

	void render() override
	{
		// Update the UBO's data:
		auto* mainWnd = gvk::context().main_window();
		const auto w = mainWnd->swap_chain_extent().width;
		const auto halfW = w * 0.5f;
		const auto h = mainWnd->swap_chain_extent().height;
		
		MatricesForUbo uboVS{};
		uboVS.projection = glm::perspective(glm::radians(60.0f), w * 0.5f / h, 0.1f, 256.0f);
		uboVS.model = glm::translate(glm::mat4{1.0f}, glm::vec3(0.0f, 0.0f, -3.0));
		uboVS.model = uboVS.model * glm::rotate(glm::mat4{1.0f}, glm::radians(gvk::time().time_since_start() * mRotationSpeed * 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Update the buffer:
		const auto ifi = mainWnd->current_in_flight_index();
		mUbo[ifi]->fill(&uboVS, 0, avk::sync::not_required());
		
		const auto imgIndex = mainWnd->current_image_index();

		// The swap chain provides us with an "image available semaphore" for the current frame.
		// Only after the swapchain image has become available, we may start rendering into it.
		auto imageAvailableSemaphore = mainWnd->consume_current_image_available_semaphore();
		
		// Record a command buffer and submit it:
		auto& commandPool = gvk::context().get_command_pool_for_single_use_command_buffers(*mQueue);
	
		auto commandBuffer = commandPool->alloc_command_buffer(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		commandBuffer->begin_recording();
		
		// Image memory barrier to make sure that compute shader writes are finished before sampling from the texture
		commandBuffer->establish_image_memory_barrier(
			mTargetImageAndSampler->get_image_view()->get_image(),
			avk::pipeline_stage::compute_shader, 
			avk::pipeline_stage::fragment_shader,
			avk::memory_access::shader_buffers_and_images_write_access, 
			avk::memory_access::shader_buffers_and_images_read_access
		);

		// Prepare some stuff:
		auto vpLeft = vk::Viewport{0.0f, 0.0f, halfW, static_cast<float>(h)};
		auto vpRight = vk::Viewport{halfW, 0.0f, halfW, static_cast<float>(h)};

		// Begin drawing:
		commandBuffer->begin_render_pass_for_framebuffer(mGraphicsPipeline->get_renderpass(), mainWnd->backbuffer_at_index(imgIndex));
		commandBuffer->bind_pipeline(avk::const_referenced(mGraphicsPipeline));

		// Draw left viewport:
		commandBuffer->handle().setViewport(0, 1, &vpLeft);

		commandBuffer->bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mUbo[ifi]),
			avk::descriptor_binding(0, 1, mInputImageAndSampler)
		}));
		commandBuffer->draw_indexed(avk::const_referenced(mIndexBuffer), avk::const_referenced(mVertexBuffer));

		// Draw right viewport (post compute)
		commandBuffer->handle().setViewport(0, 1, &vpRight);
		commandBuffer->bind_descriptors(mGraphicsPipeline->layout(), mDescriptorCache.get_or_create_descriptor_sets({
			avk::descriptor_binding(0, 0, mUbo[ifi]),
			avk::descriptor_binding(0, 1, mTargetImageAndSampler)
		}));
		commandBuffer->draw_indexed(avk::const_referenced(mIndexBuffer), avk::const_referenced(mVertexBuffer));

		commandBuffer->end_render_pass();
		commandBuffer->end_recording();
		
		mQueue->submit(commandBuffer, imageAvailableSemaphore);
		mainWnd->handle_lifetime(avk::owned(commandBuffer));
	}

private: // v== Member variables ==v

	avk::queue* mQueue;
	avk::buffer mVertexBuffer;
	avk::buffer mIndexBuffer;
	std::vector<avk::buffer> mUbo;
	avk::image_sampler mInputImageAndSampler;
	avk::image_sampler mTargetImageAndSampler;
	avk::descriptor_cache mDescriptorCache;

	avk::graphics_pipeline mGraphicsPipeline;

	std::vector<avk::compute_pipeline> mComputePipelines;
	avk::fence mComputeFence;

	float mRotationSpeed;
}; // compute_image_processing_app

int main() // <== Starting point ==
{
	try {
		// Create a window and open it
		auto* mainWnd = gvk::context().create_window("Compute Image Effects Example");
		mainWnd->set_resolution({ 1600, 800 });
		mainWnd->set_presentaton_mode(gvk::presentation_mode::mailbox);
		mainWnd->set_number_of_concurrent_frames(3u);
		mainWnd->open();

		auto& singleQueue = gvk::context().create_queue({}, avk::queue_selection_preference::versatile_queue, mainWnd);
		mainWnd->add_queue_family_ownership(singleQueue);
		mainWnd->set_present_queue(singleQueue);
		
		// Create an instance of our main avk::element which contains all the functionality:
		auto app = compute_image_processing_app(singleQueue);
		// Create another element for drawing the UI with ImGui
		auto ui = gvk::imgui_manager(singleQueue);

		// GO:
		gvk::start(
			gvk::application_name("Gears-Vk + Auto-Vk Example: Compute Image Effects Example"),
			mainWnd,
			app,
			ui
		);
	}
	catch (gvk::logic_error&) {}
	catch (gvk::runtime_error&) {}
	catch (avk::logic_error&) {}
	catch (avk::runtime_error&) {}
}
